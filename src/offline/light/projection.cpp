#include <rad/offline/render/light.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class Projection final : public Light {
 public:
  Projection(BuildContext* ctx, const ConfigNode& cfg) {
    _flag = (UInt32)LightType::DeltaPosition;
    const Matrix4& toWorld = ctx->GetShapeMatrix();
    _toWorld = Transform(toWorld);
    _irradiance = cfg.ReadTexture(*ctx, "irradiance", Color(Float(1)));
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

 private:
  Unique<Texture<Color>> _irradiance;
  Spectrum _scale;
  Transform _toWorld;
  Transform _toClip;
};

}  // namespace Rad

RAD_FACTORY_LIGHT_DECLARATION(Projection, "projection");
