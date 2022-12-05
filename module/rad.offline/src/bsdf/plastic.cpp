#include <rad/offline/render/bsdf.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/warp.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/render/fresnel.h>

namespace Rad {

/**
 * @brief 具有内部散射的光滑塑料材质，使用菲涅耳反射和透射系数来提供与方向相关的镜面反射和漫反射分量
 */
class Plastic final : public Bsdf {
 public:
  Plastic(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Diffuse | BsdfType::Reflection | BsdfType::Delta;
    Float intIor = cfg.ReadOrDefault("int_ior", Float(1.49f));      //内部折射率
    Float extIor = cfg.ReadOrDefault("ext_ior", Float(1.000277f));  //外部折射率
    _eta = intIor / extIor;
    _diffuse = ConfigNodeReadTexture(ctx, cfg, "diffuse", Color24f(Float32(0.5)));
    _specular = ConfigNodeReadTexture(ctx, cfg, "specular", Color24f(1));
    _noLinear = cfg.ReadOrDefault("no_linear", false);
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
  ~Plastic() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
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
    if (context.IsEnable(BsdfType::Delta) && context.IsEnable(BsdfType::Diffuse)) {
      probSpec = probSpec / (probSpec + probDiff);
    } else {
      probSpec = context.IsEnable(BsdfType::Delta) ? Float(1) : Float(0);
    }
    probDiff = 1 - probSpec;
    BsdfSampleResult bsr{};
    bsr.Eta = 1;
    Spectrum f;
    if (lobeXi < probSpec) {
      bsr.Wo = Fresnel::Reflect(si.Wi);
      bsr.Pdf = probSpec;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Delta;
      Spectrum spec = Color24fToSpectrum(_specular->Eval(si));
      f = Spectrum(spec * fi);
    } else {
      bsr.Wo = Warp::SquareToCosineHemisphere(dirXi);
      bsr.Pdf = Warp::SquareToCosineHemispherePdf(bsr.Wo) * probDiff;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Diffuse;
      Float cosThetaO = Frame::CosTheta(bsr.Wo);
      Float fo = std::get<0>(Fresnel::Dielectric(cosThetaO, Float(_eta)));
      Spectrum diff = Color24fToSpectrum(_diffuse->Eval(si));
      auto num = diff * (1 / Math::PI) * _invEta2 * (1 - fi) * (1 - fo) * cosThetaO;
      if (_noLinear) {
        f = Spectrum(num.cwiseProduct((Spectrum::Constant(1) - (diff * _fdrInt)).cwiseInverse()));
      } else {
        f = Spectrum(num / (1 - _fdrInt));
      }
    }
    return std::make_pair(bsr, f);
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return Spectrum(0);
    }
    Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
    Float fo = std::get<0>(Fresnel::Dielectric(cosThetaO, Float(_eta)));
    Spectrum diff = Color24fToSpectrum(_diffuse->Eval(si));
    auto num = diff * (1 / Math::PI) * _invEta2 * (1 - fi) * (1 - fo) * cosThetaO;
    Spectrum f;
    if (_noLinear) {
      f = Spectrum(num.cwiseProduct((Spectrum::Constant(1) - (diff * _fdrInt)).cwiseInverse()));
    } else {
      f = Spectrum(num / (1 - _fdrInt));
    }
    return f;
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return 0;
    }
    Float probDiff = 1;
    if (context.IsEnable(BsdfType::Delta)) {
      Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
      Float probSpec = fi * _specularSampleWeight;
      probDiff = (1 - fi) * (1 - _specularSampleWeight);
      probDiff = probDiff / (probDiff + probSpec);
    }
    Float pdf = Warp::SquareToCosineHemispherePdf(wo) * probDiff;
    return pdf;
  }

 private:
  Float _eta;
  Unique<TextureRGB> _diffuse;
  Unique<TextureRGB> _specular;
  bool _noLinear;
  Float _fdrInt;
  Float _specularSampleWeight;
  Float _invEta2;
};

class PlasticFactory final : public BsdfFactory {
 public:
  PlasticFactory() : BsdfFactory("plastic") {}
  ~PlasticFactory() noexcept override = default;
  Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Plastic>(ctx, cfg);
  }
};

Unique<BsdfFactory> _FactoryCreatePlasticFunc_() {
  return std::make_unique<PlasticFactory>();
}

}  // namespace Rad
