#include <rad/offline/render/light.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/shape.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/warp.h>

namespace Rad {

/**
 * @brief 点光源是一种解析光源，在现实生活中不存在，它从一个点将辐射均匀发射到整个空间中
 * 因为点光源的表面积为0，因此它造成的阴影边缘非常硬
 */
class Point final : public Light {
 public:
  Point(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) {
    _flag = (UInt32)LightType::DeltaPosition;
    Transform transform(toWorld);
    Vector3 position = cfg.ReadOrDefault("position", Vector3(0, 0, 0));
    _worldPos = transform.ApplyAffineToWorld(position);
    Color24f intensity = cfg.ReadOrDefault("intensity", Color24f(1, 1, 1));
    _intensity = Color24fToSpectrum(intensity);
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
    return 0;
  }

  std::pair<Ray, Spectrum> SampleRay(const Vector2& xi2, const Vector2& xi3) const override {
    auto li = _intensity * 4 * Math::PI;
    Ray ray{_worldPos, Warp::SquareToUniformSphere(xi3), 0, std::numeric_limits<Float>::max()};
    return std::make_pair(ray, Spectrum(li));
  }

  std::tuple<Ray, Spectrum, PositionSampleResult, Float, Float> SampleLe(
      const Vector2& xi2,
      const Vector2& xi3) const override {
    auto [psr, pdfPos] = SamplePosition(xi2);
    Vector3 local = Warp::SquareToUniformSphere(xi3);
    Float pdfDir = Warp::SquareToUniformSpherePdf();
    SurfaceInteraction si(psr);
    Ray ray = si.SpawnRay(si.ToWorld(local));
    Spectrum le = _intensity;
    return std::make_tuple(ray, le, psr, pdfPos, pdfDir);
  }

  std::pair<Float, Float> PdfLe(const PositionSampleResult& psr, const Vector3& dir) const override {
    Float pdfPos = 0;
    Float pdfDir = Warp::SquareToUniformSpherePdf();
    return std::make_pair(pdfPos, std::abs(pdfDir));
  }

 private:
  Vector3 _worldPos;
  Spectrum _intensity;
};

class PointFactory final : public LightFactory {
 public:
  PointFactory() : LightFactory("point") {}
  ~PointFactory() noexcept override = default;
  Unique<Light> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<Point>(ctx, toWorld, cfg);
  }
};

Unique<LightFactory> _FactoryCreatePointFunc_() {
  return std::make_unique<PointFactory>();
}

}  // namespace Rad
