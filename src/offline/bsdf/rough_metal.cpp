#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/render/microfacet.h>
#include <rad/offline/render/fresnel.h>

namespace Rad {

class RoughMetal final : public Bsdf {
 public:
  RoughMetal(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Glossy | BsdfType::Reflection;
    _reflectance = cfg.ReadTexture(*ctx, "reflectance", Color(Float(1)));
    _eta = cfg.ReadTexture(*ctx, "eta", Color(0.1999899f, 0.9220823f, 1.0998751f));
    _k = cfg.ReadTexture(*ctx, "k", Color(3.9046354f, 2.4476316f, 2.1376500f));
    if (cfg.HasNode("alpha")) {
      _alphaU = cfg.ReadTexture(*ctx, "alpha", Float(0.1));
      _alphaV = _alphaU;
    } else {
      _alphaU = cfg.ReadTexture(*ctx, "alpha_u", Float(0.1));
      _alphaV = cfg.ReadTexture(*ctx, "alpha_v", Float(0.1));
    }
    std::string distribution = cfg.ReadOrDefault("distribution", std::string("ggx"));
    if (distribution == "ggx") {
      _type = MicrofacetType::GGX;
    } else if (distribution == "beckmann") {
      _type = MicrofacetType::Beckmann;
    } else {
      Logger::Get()->warn("unknwon microfacet dist: {}", distribution);
      _type = MicrofacetType::GGX;
    }
    _isSampleVisible = cfg.ReadOrDefault("sample_visible", true);
  }
  ~RoughMetal() noexcept override = default;

  template <typename T>
  std::pair<BsdfSampleResult, Spectrum> SampleImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Glossy, BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    BsdfSampleResult bsr{};
    Vector3 m;
    std::tie(m, bsr.Pdf) = dist.Sample(si.Wi, dirXi);
    bsr.Wo = Fresnel::Reflect(si.Wi, m);
    bsr.Eta = 1;
    bsr.TypeMask = _flags;
    if (bsr.Pdf <= 0 || Frame::CosTheta(bsr.Wo) <= 0) {
      return {{}, Spectrum(0)};
    }
    bsr.Pdf /= 4 * bsr.Wo.dot(m);
    Float D = dist.D(m);
    Float G = dist.G(si.Wi, bsr.Wo, m);
    Float result = (D * G) / (4 * Frame::CosTheta(si.Wi));
    Spectrum F = Fresnel::Conductor(si.Wi.dot(m), _eta->Eval(si), _k->Eval(si));
    Spectrum r = _reflectance->Eval(si);
    return {bsr, Spectrum(r.cwiseProduct(F) * result)};
  }

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Beckmann dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
        return SampleImpl(context, si, lobeXi, dirXi, dist);
      }
      case MicrofacetType::GGX:
      default: {
        GGX dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
        return SampleImpl(context, si, lobeXi, dirXi, dist);
      }
    }
  }

  template <typename T>
  Spectrum EvalImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Glossy, BsdfType::Reflection)) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return Spectrum(0);
    }
    Vector3 wh = (wo + si.Wi).normalized();
    Spectrum eta = _eta->Eval(si), k = _k->Eval(si), r = _reflectance->Eval(si);
    Vector3 F = Fresnel::Conductor(si.Wi.dot(wh), eta, k);
    Float D = dist.D(wh);
    Float G = dist.G(si.Wi, wo, wh);
    auto f = r.cwiseProduct((F * D * G).cwiseAbs() / (cosThetaI * 4));
    Spectrum fr = Spectrum(f);
    return fr;
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Beckmann dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
        return EvalImpl(context, si, wo, dist);
      }
      case MicrofacetType::GGX:
      default: {
        GGX dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
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
    if (!context.IsEnable(BsdfType::Glossy, BsdfType::Reflection)) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return 0;
    }
    Vector3 wh = (wo + si.Wi).normalized();
    Float pdf = dist.Pdf(si.Wi, wh) / (4 * wo.dot(wh));
    return pdf;
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    switch (_type) {
      case MicrofacetType::Beckmann: {
        Beckmann dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
        return PdfImpl(context, si, wo, dist);
      }
      case MicrofacetType::GGX:
      default: {
        GGX dist{{_alphaU->Eval(si), _alphaV->Eval(si)}, _isSampleVisible};
        return PdfImpl(context, si, wo, dist);
      }
    }
  }

 private:
  Share<Texture<Color>> _reflectance;
  Share<Texture<Color>> _eta;
  Share<Texture<Color>> _k;
  Share<Texture<Float>> _alphaU;
  Share<Texture<Float>> _alphaV;
  MicrofacetType _type;
  bool _isSampleVisible;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(RoughMetal, "rough_metal");
