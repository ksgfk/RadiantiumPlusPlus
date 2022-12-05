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
 * @brief 投影光源是一种解析光源，它像透视相机一样，从一个点向特定方向投射光线
 *
 */
class Projection final : public Light {
 public:
  Projection(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) {
    _flag = (UInt32)LightType::DeltaPosition;
    _toWorld = Transform(toWorld);
    _irradiance = ConfigNodeReadTexture(ctx, cfg, "irradiance", Color24f(1));
    Vector3 irScale = cfg.ReadOrDefault("scale", Vector3(1, 1, 1));
    _scale = Spectrum(irScale);
    Float fov = cfg.ReadOrDefault("fov", Float(90));
    Float aspect = _irradiance->Width() / (Float)_irradiance->Height();
    Float far = 10000;
    Float near = Float(0.0001);
    Eigen::DiagonalMatrix<Float, 3> scale(-0.5f, -0.5f * aspect, 1);
    Eigen::Translation<Float, 3> translate(-1, -1 / aspect, 0);
    Float recip = 1 / (far - near);
    Float cot = 1 / std::tan(Math::Radian(fov / 2));
    Matrix4 perspective;
    perspective << cot, 0, 0, 0,
        0, cot, 0, 0,
        0, 0, far * recip, -near * far * recip,
        0, 0, 1, 0;
    Matrix4 toClip = scale * translate * perspective;
    _toClip = Transform(toClip);

    Vector3 pMin = _toClip.ApplyAffineToLocal(Vector3(0, 0, 0));
    Vector3 pMax = _toClip.ApplyAffineToLocal(Vector3(1, 1, 0));
    BoundingBox2 rect(pMin.head<2>() / pMin.z(), pMax.head<2>() / pMax.z());
    _cameraArea = 1 / rect.volume();

    _cosWidth = pMax.z();
  }
  ~Projection() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    return 0;
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    Vector3 local = _toWorld.ApplyAffineToLocal(ref.P);
    Vector2 uv = _toClip.ApplyAffineToWorld(local).head<2>();
    bool inRange = uv.x() >= 0 && uv.x() <= 1 && uv.y() >= 0 && uv.y() <= 1 && local.z() > 0;
    if (!inRange) {
      return {{}, Spectrum(0)};
    }
    DirectionSampleResult dsr{};
    dsr.P = _toWorld.TranslationToWorld();
    dsr.N = _toWorld.ApplyLinearToWorld(Vector3(0, 0, 1)).normalized();
    dsr.UV = uv;
    dsr.Pdf = 1;
    dsr.IsDelta = true;
    dsr.Dir = dsr.P - ref.P;
    Float squaredNorm = dsr.Dir.squaredNorm();
    dsr.Dist = std::sqrt(squaredNorm);
    dsr.Dir *= Math::Rcp(dsr.Dist);
    Spectrum irradiance(_irradiance->Eval(SurfaceInteraction(dsr)));
    Spectrum result = Spectrum(irradiance.cwiseProduct(_scale) * Math::PI / (Math::Sqr(local.z()) * (-dsr.Dir).dot(dsr.N)));
    return std::make_pair(dsr, result);
  }

  Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const override {
    return 0;
  }

  std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const override {
    PositionSampleResult psr{};
    psr.P = _toWorld.TranslationToWorld();
    psr.N = _toWorld.ApplyLinearToWorld(Vector3(0, 0, 1)).normalized();
    psr.UV = Vector2::Constant(Float(0.5));
    psr.Pdf = 1;
    psr.IsDelta = true;
    return std::make_pair(psr, Float(1));
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    return 0;
  }

  std::pair<Ray, Spectrum> SampleRay(const Vector2& xi2, const Vector2& xi3) const override {
    Vector2 uv = xi3;  //这里得重要性采样
    Vector3 nearP = _toClip.ApplyAffineToLocal(Vector3(uv.x(), uv.y(), 0));
    Vector3 nearDir = nearP.normalized();
    Ray ray{_toWorld.TranslationToWorld(), _toWorld.ApplyLinearToWorld(nearDir), 0, std::numeric_limits<Float>::max()};
    SurfaceInteraction si{};
    si.UV = uv;
    auto li = _irradiance->Eval(si) * Math::PI * _cameraArea;
    return std::make_pair(ray, Spectrum(li));
  }

  std::tuple<Ray, Spectrum, PositionSampleResult, Float, Float> SampleLe(
      const Vector2& xi2,
      const Vector2& xi3) const override {
    auto [psr, pdfPos] = SamplePosition(xi2);
    Vector2 uv = xi3;  //这里得重要性采样
    Vector3 nearP = _toClip.ApplyAffineToLocal(Vector3(uv.x(), uv.y(), 0));
    Vector3 nearDir = nearP.normalized();
    Ray ray{_toWorld.TranslationToWorld(), _toWorld.ApplyLinearToWorld(nearDir), 0, std::numeric_limits<Float>::max()};
    SurfaceInteraction si{};
    si.UV = uv;
    auto le = _irradiance->Eval(si);
    Float pdfDir = 1 / (_cameraArea * Math::PI);
    return std::make_tuple(ray, Spectrum(le), psr, pdfPos, pdfDir);
  }

  std::pair<Float, Float> PdfLe(const PositionSampleResult& psr, const Vector3& dir) const override {
    Vector3 refP = _toWorld.ApplyAffineToLocal(psr.P);
    Vector3 ndcPos = _toClip.ApplyAffineToWorld(refP);
    Float pdfDir;
    if (ndcPos.x() < 0 || ndcPos.x() >= 1 || ndcPos.y() < 0 || ndcPos.y() >= 1 || ndcPos.z() <= 0) {
      pdfDir = 0;
    } else {
      pdfDir = 1 / (_cameraArea * Math::PI);
    }
    Float pdfPos = 0;
    return std::make_pair(pdfPos, std::abs(pdfDir));
  }

 private:
  Unique<TextureRGB> _irradiance;
  Spectrum _scale;
  Transform _toWorld;
  Transform _toClip;
  Float _cameraArea;
  Float _cosWidth;
};

class ProjectionFactory final : public LightFactory {
 public:
  ProjectionFactory() : LightFactory("projection") {}
  ~ProjectionFactory() noexcept override = default;
  Unique<Light> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<Projection>(ctx, toWorld, cfg);
  }
};

Unique<LightFactory> _FactoryCreateProjectionFunc_() {
  return std::make_unique<ProjectionFactory>();
}

}  // namespace Rad
