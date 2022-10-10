#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/render/microfacet.h>
#include <rad/offline/render/fresnel.h>

using namespace Rad::Math;

namespace Rad {

class RoughGlass final : public Bsdf {
 public:
  RoughGlass(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Glossy | BsdfType::Reflection | BsdfType::Transmission;
    _reflectance = cfg.ReadTexture(*ctx, "reflectance", Color(Float(1)));
    _transmittance = cfg.ReadTexture(*ctx, "transmittance", Color(Float(1)));
    Float intIor = cfg.ReadOrDefault("int_ior", 1.5046f);    //内部折射率
    Float extIor = cfg.ReadOrDefault("ext_ior", 1.000277f);  //外部折射率
    _eta = intIor / extIor;
    _invEta = extIor / intIor;
    std::string distribution = cfg.ReadOrDefault("distribution", std::string("ggx"));
    if (distribution == "ggx") {
      _type = MicrofacetType::GGX;
    } else if (distribution == "beckmann") {
      _type = MicrofacetType::Beckmann;
    } else {
      Logger::Get()->warn("unknwon microfacet dist: {}", distribution);
      _type = MicrofacetType::GGX;
    }
    if (cfg.HasNode("alpha")) {
      _alphaU = cfg.ReadTexture(*ctx, "alpha", Float32(0.1));
      _alphaV = cfg.ReadTexture(*ctx, "alpha", Float32(0.1));
    } else {
      _alphaU = cfg.ReadTexture(*ctx, "alpha_u", Float32(0.1));
      _alphaV = cfg.ReadTexture(*ctx, "alpha_v", Float32(0.1));
    }
    _isSampleVisible = cfg.ReadOrDefault("sample_visible", true);
  }
  ~RoughGlass() noexcept override = default;

  template <typename T>
  std::pair<BsdfSampleResult, Spectrum> SampleImpl(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi,
      const MicrofacetDistribution<T>& dist) const {
    if (!context.IsEnable(BsdfType::Glossy)) {
      return {{}, Spectrum(0)};
    }
    bool hasReflection = context.IsEnable(BsdfType::Reflection);
    bool hasTransmission = context.IsEnable(BsdfType::Transmission);
    BsdfSampleResult bsr{};
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI == 0) {
      return {{}, Spectrum(0)};
    }
    auto [wh, pdf] = dist.Sample(MulSign(si.Wi, cosThetaI), dirXi);
    if (pdf == 0) {
      return {{}, Spectrum(0)};
    }
    auto [F, cosThetaT, etaIT, etaTI] = Fresnel::Dielectric(si.Wi.dot(wh), _eta);
    bool selectedR;
    if (hasReflection && hasTransmission) {
      selectedR = lobeXi <= F;
      // bsr.Pdf = pdf * (selectedR ? F : (1 - F));
    } else {
      if (hasReflection || hasTransmission) {
        selectedR = hasReflection;
        // bsr.Pdf = 1;
      } else {
        return {{}, Spectrum(0)};
      }
    }
    bsr.Wo = selectedR ? Fresnel::Reflect(si.Wi, wh) : Fresnel::Refract(si.Wi, wh, cosThetaT, etaTI);
    bool isReflect = Frame::IsSameHemisphere(si.Wi, bsr.Wo);
    bsr.Eta = isReflect ? 1 : etaIT;
    bsr.TypeMask = BsdfType::Glossy | (isReflect ? BsdfType::Reflection : BsdfType::Transmission);
    // Float dwhdwo = 0;
    // Spectrum result;
    // if (selectedR) {
    //   bsr.Wo = Fresnel::Reflect(si.Wi, wh);
    //   dwhdwo = Rcp(bsr.Wo.dot(wh) * 4);
    //   Float D = dist.D(wh);
    //   Float G = dist.G(si.Wi, bsr.Wo, wh);
    //   Float brdf = std::abs(F * D * G / (cosThetaI * 4));
    //   Spectrum r = _reflectance->Eval(si);
    //   result = Spectrum(r * brdf);
    // }
    // if (selectedT) {
    //   bsr.Wo = Fresnel::Refract(si.Wi, wh, cosThetaT, etaTI);
    //   dwhdwo = (Sqr(bsr.Eta) * bsr.Wo.dot(wh)) /
    //            Sqr(si.Wi.dot(wh) + bsr.Eta * bsr.Wo.dot(wh));
    //   Float scale = context.Mode == TransportMode::Radiance ? Sqr(etaTI) : 1;
    //   Float D = dist.D(wh);
    //   Float G = dist.G(si.Wi, bsr.Wo, wh);
    //   Float idm = si.Wi.dot(wh);
    //   Float odm = bsr.Wo.dot(wh);
    //   Float btdf = std::abs((scale * (1 - F) * D * G * Sqr(bsr.Eta) * idm * odm) /
    //                         (cosThetaI * Sqr(idm + bsr.Eta * odm)));
    //   Spectrum t = _transmittance->Eval(si);
    //   result = Spectrum(t * btdf);
    // }
    // bsr.Pdf *= std::abs(dwhdwo);
    bsr.Pdf = PdfImpl(context, si, bsr.Wo, dist);
    Spectrum result = EvalImpl(context, si, bsr.Wo, dist);
    return std::make_pair(bsr, result);
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
    if (!context.IsEnable(BsdfType::Glossy)) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI == 0) {
      return Spectrum(0);
    }
    bool hasReflection = context.IsEnable(BsdfType::Reflection);
    bool hasTransmission = context.IsEnable(BsdfType::Transmission);
    bool reflect = cosThetaI * cosThetaO > 0;
    Float eta = cosThetaI > 0 ? _eta : _invEta;
    Float invEta = cosThetaI > 0 ? _invEta : _eta;
    Vector3 m = (si.Wi + wo * (reflect ? 1 : eta)).normalized();
    m = MulSign(m, Frame::CosTheta(m));
    Float D = dist.D(m);
    Float F = std::get<0>(Fresnel::Dielectric(si.Wi.dot(m), _eta));
    Float G = dist.G(si.Wi, wo, m);
    Spectrum result(0);
    if (hasReflection && reflect) {
      Float brdf = std::abs(F * D * G / (cosThetaI * 4));
      Spectrum r(_reflectance->Eval(si));
      result = Spectrum(r * brdf);
    }
    if (hasTransmission && !reflect) {
      Float scale = context.Mode == TransportMode::Radiance ? Sqr(invEta) : 1;
      Float idm = si.Wi.dot(m);
      Float odm = wo.dot(m);
      Float btdf = std::abs((scale * (1 - F) * D * G * eta * eta * idm * odm) /
                            (cosThetaI * Sqr(idm + eta * odm)));
      Spectrum t(_transmittance->Eval(si));
      result = Spectrum(t * btdf);
    }
    return result;
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
    if (!context.IsEnable(BsdfType::Glossy)) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI == 0) {
      return 0;
    }
    bool hasReflection = context.IsEnable(BsdfType::Reflection);
    bool hasTransmission = context.IsEnable(BsdfType::Transmission);
    bool reflect = cosThetaI * cosThetaO > 0;
    Float eta = cosThetaI > 0 ? _eta : _invEta;
    Vector3 m = (si.Wi + wo * (reflect ? 1 : eta)).normalized();
    m = MulSign(m, Frame::CosTheta(m));
    Float idm = si.Wi.dot(m);
    Float odm = wo.dot(m);
    if (idm * cosThetaI <= 0 || odm * cosThetaO <= 0) {
      return 0;
    }
    Float dwhdwo = reflect ? Rcp(odm * 4) : ((eta * eta * odm) / (Sqr(idm + eta * odm)));
    Float pdf = dist.Pdf(MulSign(si.Wi, cosThetaI), m);
    if (hasReflection && hasTransmission) {
      Float F = std::get<0>(Fresnel::Dielectric(idm, _eta));
      pdf *= reflect ? F : (1 - F);
    }
    Float result = std::abs(pdf * dwhdwo);
    return result;
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
  Unique<TextureRGB> _reflectance;
  Unique<TextureRGB> _transmittance;
  Float _eta;
  Float _invEta;
  MicrofacetType _type;
  Unique<TextureGray> _alphaU;
  Unique<TextureGray> _alphaV;
  bool _isSampleVisible;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(RoughGlass, "rough_glass");
