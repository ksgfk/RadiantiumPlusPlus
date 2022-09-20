#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>

namespace Rad {

class Diffuse final : public Bsdf {
 public:
  Diffuse(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Diffuse | BsdfType::Reflection;
    _reflectance = cfg.ReadTexture(*ctx, "reflectance", Color(Float(0.5)));
  }
  ~Diffuse() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    BsdfSampleResult bsr{};
    bsr.Wo = Warp::SquareToCosineHemisphere(dirXi);
    bsr.Pdf = Warp::SquareToCosineHemispherePdf(bsr.Wo);
    bsr.Eta = Float(1);
    bsr.TypeMask = _flags;
    Float cosThetaO = Frame::CosTheta(bsr.Wo);
    Spectrum reflectance = Spectrum(_reflectance->Eval(si));
    auto f = reflectance * (1 / Math::PI) * cosThetaO;
    return std::make_pair(bsr, Spectrum(f));
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
    Spectrum reflectance = Spectrum(_reflectance->Eval(si));
    auto f = reflectance * (1 / Math::PI) * cosThetaO;
    return Spectrum(f);
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
    Float pdf = Warp::SquareToCosineHemispherePdf(wo);
    return pdf;
  }

 private:
  Share<Texture<Color>> _reflectance;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(Diffuse, "diffuse");
