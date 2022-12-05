#include <rad/offline/render/bsdf.h>

#include <rad/core/config_node.h>
#include <rad/core/logger.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/warp.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/render/fresnel.h>
#include <rad/offline/render/microfacet.h>

namespace Rad {

/**
 * @brief 具有内部散射的粗糙塑料材质
 * （WIP，没有完全模拟真实内部散射情况，只是看起来像）
 */
class RoughPlastic final : public Bsdf {
 public:
  RoughPlastic(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Diffuse | BsdfType::Reflection | BsdfType::Delta;
    Float intIor = cfg.ReadOrDefault("int_ior", Float(1.49f));      //内部折射率
    Float extIor = cfg.ReadOrDefault("ext_ior", Float(1.000277f));  //外部折射率
    _eta = intIor / extIor;
    _diffuse = ConfigNodeReadTexture(ctx, cfg, "diffuse", Color24f(Float32(0.5)));
    _specular = ConfigNodeReadTexture(ctx, cfg, "specular", Color24f(1));
    _noLinear = cfg.ReadOrDefault("no_linear", false);
    std::string distribution = cfg.ReadOrDefault("distribution", std::string("ggx"));
    _alpha = ConfigNodeReadTexture(ctx, cfg, "alpha", Float32(0.1));
    if (distribution == "ggx") {
      _type = MicrofacetType::GGX;
    } else if (distribution == "beckmann") {
      _type = MicrofacetType::Beckmann;
    } else {
      Logger::Get()->warn("unknwon microfacet dist: {}", distribution);
      _type = MicrofacetType::GGX;
    }
    _isSampleVisible = cfg.ReadOrDefault("sample_visible", true);
    _fdrInt = Fresnel::DiffuseReflectance(Float(1) / _eta);
    _invEta2 = Float(1) / (_eta * _eta);
    Float64 diffMean = 0;
    for (UInt32 y = 0; y < _diffuse->Height(); y++) {
      for (UInt32 x = 0; x < _diffuse->Width(); x++) {
        diffMean += _diffuse->Read(x, y).Luminance();
      }
    }
    diffMean /= (_diffuse->Height() * _diffuse->Width());
    Float64 specMean = 0;
    for (UInt32 y = 0; y < _specular->Height(); y++) {
      for (UInt32 x = 0; x < _specular->Width(); x++) {
        specMean += _specular->Read(x, y).Luminance();
      }
    }
    specMean /= (_specular->Height() * _specular->Width());
    _specularSampleWeight = (Float)(specMean / (diffMean + specMean));
  }
  ~RoughPlastic() noexcept override = default;

  template <typename T>
  std::pair<BsdfSampleResult, Spectrum> SampleImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
    Float probSpec = fi * _specularSampleWeight;
    Float probDiff = (1 - fi) * (1 - _specularSampleWeight);
    if (context.IsEnable(BsdfType::Glossy) && context.IsEnable(BsdfType::Diffuse)) {
      probSpec = probSpec / (probSpec + probDiff);
    } else {
      probSpec = context.IsEnable(BsdfType::Glossy) ? Float(1) : Float(0);
    }
    probDiff = 1 - probSpec;
    BsdfSampleResult bsr{};
    bsr.Eta = 1;
    Spectrum f;
    if (lobeXi < probSpec) {
      auto [wh, pdf] = dist.Sample(si.Wi, dirXi);
      if (pdf <= 0) {
        return {bsr, Spectrum(0)};
      }
      bsr.Wo = Fresnel::Reflect(si.Wi, wh);
      bsr.TypeMask = BsdfType::Glossy | BsdfType::Reflection;
    } else {
      bsr.Wo = Warp::SquareToCosineHemisphere(dirXi);
      bsr.TypeMask = BsdfType::Diffuse | BsdfType::Reflection;
    }
    bsr.Pdf = PdfImpl(context, si, bsr.Wo, dist);
    if (bsr.Pdf <= 0) {
      return {bsr, Spectrum(0)};
    }
    Spectrum fs = EvalImpl(context, si, bsr.Wo, dist);
    return std::make_pair(bsr, fs);
  }

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Float alpha = _alpha->Eval(si);
        Beckmann dist{{alpha, alpha}, _isSampleVisible};
        return SampleImpl(context, si, lobeXi, dirXi, dist);
      }
      case MicrofacetType::GGX:
      default: {
        Float alpha = _alpha->Eval(si);
        GGX dist{{alpha, alpha}, _isSampleVisible};
        return SampleImpl(context, si, lobeXi, dirXi, dist);
      }
    }
  }

  //没有使用微表面来估计表面透射率, 结果会亮一些
  template <typename T>
  Spectrum EvalImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Reflection)) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return Spectrum(0);
    }
    Spectrum spec = Color24fToSpectrum(_specular->Eval(si));
    Spectrum diff = Color24fToSpectrum(_diffuse->Eval(si));
    Spectrum specular(0);
    Vector3 wh = (wo + si.Wi).normalized();
    Float F = std::get<0>(Fresnel::Dielectric(si.Wi.dot(wh), _eta));
    if (context.IsEnable(BsdfType::Glossy)) {
      Float D = dist.D(wh);
      Float G = dist.G(si.Wi, wo, wh);
      specular = Spectrum(spec * D * G * F / (4 * cosThetaI));
    }
    Spectrum diffuse(0);
    if (context.IsEnable(BsdfType::Diffuse)) {
      // Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
      // Float fo = std::get<0>(Fresnel::Dielectric(cosThetaO, Float(_eta)));
      Float fi = F;
      Float fo = std::get<0>(Fresnel::Dielectric(wo.dot(wh), Float(_eta)));
      auto num = diff * (1 / Math::PI) * _invEta2 * (1 - fi) * (1 - fo) * cosThetaO;
      if (_noLinear) {
        diffuse = Spectrum(num.cwiseProduct((Spectrum::Constant(1) - (diff * _fdrInt)).cwiseInverse()));
      } else {
        diffuse = Spectrum(num / (1 - _fdrInt));
      }
    }
    Spectrum result(specular + diffuse);
    return result;
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Float alpha = _alpha->Eval(si);
        Beckmann dist{{alpha, alpha}, _isSampleVisible};
        return EvalImpl(context, si, wo, dist);
      }
      case MicrofacetType::GGX:
      default: {
        Float alpha = _alpha->Eval(si);
        GGX dist{{alpha, alpha}, _isSampleVisible};
        return EvalImpl(context, si, wo, dist);
      }
    }
  }

  template <typename T>
  Float PdfImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Reflection)) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return 0;
    }
    Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
    Float probSpec = fi * _specularSampleWeight;
    Float probDiff = (1 - fi) * (1 - _specularSampleWeight);
    if (context.IsEnable(BsdfType::Glossy) && context.IsEnable(BsdfType::Diffuse)) {
      probSpec = probSpec / (probSpec + probDiff);
    } else {
      probSpec = context.IsEnable(BsdfType::Glossy) ? Float(1) : Float(0);
    }
    probDiff = 1 - probSpec;
    Float specPdf = 0;
    if (context.IsEnable(BsdfType::Glossy)) {
      Vector3 wh = (wo + si.Wi).normalized();
      specPdf = dist.Pdf(si.Wi, wh) / (4 * wo.dot(wh)) * probSpec;
    }
    Float diffPdf = 0;
    if (context.IsEnable(BsdfType::Diffuse)) {
      diffPdf = Warp::SquareToCosineHemispherePdf(wo) * probDiff;
    }
    return specPdf + diffPdf;
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Float alpha = _alpha->Eval(si);
        Beckmann dist{{alpha, alpha}, _isSampleVisible};
        return PdfImpl(context, si, wo, dist);
      }
      case MicrofacetType::GGX:
      default: {
        Float alpha = _alpha->Eval(si);
        GGX dist{{alpha, alpha}, _isSampleVisible};
        return PdfImpl(context, si, wo, dist);
      }
    }
  }

 private:
  Float _eta;
  Unique<TextureRGB> _diffuse;
  Unique<TextureRGB> _specular;
  Unique<TextureMono> _alpha;
  bool _noLinear;
  Float _fdrInt;
  Float _specularSampleWeight;
  Float _invEta2;
  bool _isSampleVisible;
  MicrofacetType _type;
};

class RoughPlasticFactory final : public BsdfFactory {
 public:
  RoughPlasticFactory() : BsdfFactory("rough_plastic") {}
  ~RoughPlasticFactory() noexcept override = default;
  Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<RoughPlastic>(ctx, cfg);
  }
};

Unique<BsdfFactory> _FactoryCreateRoughPlasticFunc_() {
  return std::make_unique<RoughPlasticFactory>();
}

}  // namespace Rad
