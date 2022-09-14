#include <radiantium/bsdf.h>

#include <radiantium/asset.h>
#include <radiantium/warp.h>
#include <radiantium/texture.h>
#include <radiantium/math_ext.h>
#include <radiantium/factory.h>
#include <radiantium/config_node.h>

#include <memory>

namespace rad {

class SmoothDiffuse : public IBsdf {
 public:
  SmoothDiffuse(std::unique_ptr<ITexture> reflectance) : _reflectance(std::move(reflectance)) {}
  ~SmoothDiffuse() noexcept override {}

  UInt32 Flags() const override { return (BsdfType::Diffuse | BsdfType::Reflection); }

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobe, const Vec2& dir) const override {
    Float cosThetaI = Frame::CosTheta(si.Wi);
    BsdfSampleResult bsr{};
    if (cosThetaI <= 0 || !context.IsEnableType(Flags())) {
      return {bsr, Spectrum(0)};
    }
    bsr.Wo = warp::SquareToCosineHemisphere(dir);
    bsr.Pdf = warp::SquareToUniformHemispherePdf();
    bsr.Eta = 1;
    bsr.Type = Flags();
    Spectrum value = _reflectance->Sample(si.UV);
    auto f = value / math::PI * Frame::CosPhi(bsr.Wo);
    return {bsr, Spectrum(f)};
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vec3& wo) const override {
    if (!context.IsEnableType(Flags())) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return Spectrum(0);
    }
    Spectrum value = _reflectance->Sample(si.UV);
    auto f = value / math::PI * cosThetaO;
    return Spectrum(f);
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vec3& wo) const override {
    if (!context.IsEnableType(Flags())) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return 0;
    }
    Float pdf = warp::SquareToUniformHemispherePdf();
    return pdf;
  }

 private:
  std::unique_ptr<ITexture> _reflectance;
};

}  // namespace rad

namespace rad::factory {
class DiffuseFactory : public IBsdfFactory {
 public:
  ~DiffuseFactory() noexcept override = default;
  std::string UniqueId() const override { return "diffuse"; }
  std::unique_ptr<IBsdf> Create(const BuildContext* context, const IConfigNode* config) const override {
    auto reflectance = IConfigNode::GetTexture(
        config, context, "reflectance", RgbSpectrum(Float(0.5)));
    return std::make_unique<SmoothDiffuse>(std::move(reflectance));
  }
};
std::unique_ptr<IFactory> CreateDiffuseFactory() {
  return std::make_unique<DiffuseFactory>();
}
}  // namespace rad::factory
