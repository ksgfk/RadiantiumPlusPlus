#include <rad/offline/render/light.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class DiffuseArea final : public Light {
 public:
  DiffuseArea(BuildContext* ctx, const ConfigNode& cfg) {
    _radiance = cfg.ReadTexture(*ctx, "radiance", Color(1));
  }
  ~DiffuseArea() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    Color radiance = si.ToWorld(si.Wi).dot(si.N) >= 0 ? _radiance->Eval(si) : Color(0);
    return Spectrum(radiance);
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    DirectionSampleResult dsr = _shape->SampleDirection(ref, xi);
    Color radiance = (-dsr.Dir).dot(dsr.N) >= 0
                         ? _radiance->Eval(SurfaceInteraction(dsr))
                         : Color(0);
    return std::make_pair(dsr, Spectrum(radiance));
  }
  Float PdfDirection(
      const Interaction& ref,
      const DirectionSampleResult& dsr) const override {
    Float pdf = (-dsr.Dir).dot(dsr.N) >= 0 ? _shape->PdfDirection(ref, dsr) : 0;
    return pdf;
  }

  Spectrum EvalDirection(
      const Interaction& ref,
      const DirectionSampleResult& dsr) const override {
    Color radiance = (-dsr.Dir).dot(dsr.N) >= 0
                         ? _radiance->Eval(SurfaceInteraction(dsr))
                         : Color(0);
    return Spectrum(radiance);
  }

  std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const override {
    PositionSampleResult psr = _shape->SamplePosition(xi);
    Float weight = psr.Pdf > 0 ? psr.Pdf : 0;
    return std::make_pair(psr, weight);
  }
  Float PdfPosition(const PositionSampleResult& psr) const override {
    return _shape->PdfPosition(psr);
  }

 private:
  Share<Texture<Color>> _radiance;
};

}  // namespace Rad

RAD_FACTORY_LIGHT_DECLARATION(DiffuseArea, "area");
