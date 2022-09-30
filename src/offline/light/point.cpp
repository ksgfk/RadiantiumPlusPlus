#include <rad/offline/render/light.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class Point final : public Light {
 public:
  Point(BuildContext* ctx, const ConfigNode& cfg) {
    _flag = (UInt32)LightType::DeltaPosition;
    const Matrix4& toWorld = ctx->GetShapeMatrix();
    Transform transform(toWorld);
    Vector3 position = cfg.ReadOrDefault("position", Vector3(0, 0, 0));
    _worldPos = transform.ApplyAffineToWorld(position);
    Vector3 intensity = cfg.ReadOrDefault("intensity", Vector3(1, 1, 1));
    _intensity = Spectrum(intensity);
  }
  ~Point() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    return Spectrum(0);
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    DirectionSampleResult dsr{};
    dsr.P = _worldPos;
    dsr.N = Vector3::Zero();
    dsr.UV = Vector2::Zero();
    dsr.Pdf = 1;
    dsr.IsDelta = true;
    dsr.Dir = dsr.P - ref.P;
    Float dist2 = dsr.Dir.squaredNorm();
    dsr.Dist = std::sqrt(dist2);
    Float invDist = Math::Rcp(dsr.Dist);
    dsr.Dir *= invDist;
    Vector3 power = _intensity * Math::Sqr(invDist);
    return std::make_pair(dsr, Spectrum(power));
  }

  Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const override {
    return 0;
  }

  std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const override {
    PositionSampleResult psr{};
    psr.P = _worldPos;
    psr.N = Vector3::Zero();
    psr.UV = Vector2::Zero();
    psr.Pdf = 1;
    psr.IsDelta = true;
    return std::make_pair(psr, Float(1));
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    throw RadInvalidOperationException("impossible to get pos pdf from point light");
  }

 private:
  Vector3 _worldPos;
  Spectrum _intensity;
};

}  // namespace Rad

RAD_FACTORY_LIGHT_DECLARATION(Point, "point");
