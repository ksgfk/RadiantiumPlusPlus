#include <rad/offline/render/light.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/utility/distribution.h>

using namespace Rad::Math;

namespace Rad {

/**
 * @brief 天空盒, 这是一种无限远的环境光照
 *
 */
class Skybox final : public Light {
 public:
  Skybox(BuildContext* ctx, const ConfigNode& cfg) {
    const Matrix4& toWorld = ctx->GetShapeMatrix();
    _toWorld = Transform(toWorld);
    _flag = (UInt32)LightType::Infinite;
    _map = cfg.ReadTexture(*ctx, "radiance", Color(Float(1)));
    UInt32 width = _map->Width(), height = _map->Height();
    _isUniformMap = width == 1 && height == 1;
    if (!_isUniformMap) {
      std::vector<Float> luminance;  //计算光源亮度, 用来重要性采样
      luminance.resize(width * height);
      for (UInt32 y = 0; y < height; y++) {
        //因为经纬图越靠近两极, 数据越稀疏 (相对于赤道附近), 所以越靠近两极提供的照明越少
        Float sinTheta = std::sin((y + Float(0.5)) / height * PI);
        Float v = y / (Float)height;
        UInt32 start = y * width;
        for (UInt32 x = 0; x < width; x++) {
          Float u = x / (Float)width;
          SurfaceInteraction si;
          si.UV = Vector2(u, v);
          Color color = _map->Eval(si);
          Float lum = color.Luminance();
          lum *= sinTheta;
          luminance[start + x] = lum;
        }
      }
      _dist = ContinuousDistribution2D(luminance.data(), width, height);
    }
  }
  ~Skybox() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    Vector3 v = _toWorld.ApplyLinearToLocal(-si.Wi);
    Float phi = std::atan2(v.x(), -v.z());
    phi = (phi < 0) ? (phi + 2 * PI) : phi;
    Float theta = std::acos(std::clamp(v.y(), Float(-1), Float(1)));
    Vector2 uv(phi / (2 * PI), theta / PI);
    SurfaceInteraction tsi = si;
    tsi.UV = uv;
    Color color = _map->Eval(tsi);
    return Spectrum(color);
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    Float radius = std::max(_worldSphere.Radius, (ref.P - _worldSphere.Center).norm());
    Float dist = 2 * radius;
    DirectionSampleResult dsr{};
    Color radiance;
    if (_isUniformMap) {
      Vector3 dir = Warp::SquareToUniformSphere(xi).normalized();
      Float phi = std::atan2(dir.x(), -dir.z());
      phi = (phi < 0) ? (phi + 2 * PI) : phi;
      Float theta = std::acos(std::clamp(dir.y(), Float(-1), Float(1)));
      Vector2 uv(phi / (2 * PI), theta / PI);
      Vector3 worldDir = _toWorld.ApplyLinearToWorld(dir);
      dsr.P = worldDir * dist + ref.P;
      dsr.N = -worldDir;
      dsr.UV = uv;
      dsr.Pdf = Warp::SquareToUniformSpherePdf();
      dsr.Dir = worldDir;
      dsr.Dist = dist;
      dsr.IsDelta = false;
      SurfaceInteraction evalRadiance;
      evalRadiance.UV = uv;
      radiance = _map->Eval(evalRadiance);
    } else {
      auto [worldDir, pdfDir, uv] = SampleLightDir(xi);
      if (pdfDir <= 0) {
        dsr.Pdf = 0;
        return {dsr, Spectrum(0)};
      }
      dsr.P = worldDir * dist + ref.P;
      dsr.N = -worldDir;
      dsr.UV = uv;
      dsr.Pdf = pdfDir;
      dsr.Dir = worldDir;
      dsr.Dist = dist;
      dsr.IsDelta = false;
      SurfaceInteraction evalRadiance;
      evalRadiance.UV = uv;
      radiance = _map->Eval(evalRadiance);
    }
    return std::make_pair(dsr, Spectrum(radiance));
  }

  Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const override {
    if (_isUniformMap) {
      return Warp::SquareToUniformSpherePdf();
    } else {
      Vector3 v = _toWorld.ApplyAffineToLocal(dsr.Dir);
      Float theta = std::acos(std::clamp(v.y(), Float(-1), Float(1)));
      Float sinTheta = std::sin(theta);
      if (sinTheta == 0) {
        return 0;
      }
      Float phi = std::atan2(v.x(), -v.z());
      phi = (phi < 0) ? (phi + 2 * PI) : phi;
      Vector2 uv(phi / (2 * PI), theta / PI);
      Float pdf = _dist.Pdf(uv);
      Float result = pdf / (2 * PI * PI * sinTheta);
      return result;
    }
  }

  std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const override {
    throw RadInvalidOperationException("impossible to sample pos on skybox");
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    throw RadInvalidOperationException("impossible to sample pos on skybox");
  }

  void SetScene(const Scene* scene) override {
    BoundingBox3 bound = scene->GetWorldBound();
    BoundingSphere sph = BoundingSphere::FromBox(bound);
    sph.Radius = std::max(RayEpsilon, sph.Radius * (1 + ShadowEpsilon));
    _worldSphere = sph;
  }

  std::pair<Ray, Spectrum> SampleRay(const Vector2& xi2, const Vector2& xi3) const override {
    Vector2 offset = Warp::SquareToUniformDiskConcentric(xi2);
    Float pdfDir;
    Vector2 uv;
    if (_isUniformMap) {
      pdfDir = Warp::SquareToUniformSpherePdf();
      uv = xi3;
    } else {
      Vector3 unuse;
      std::tie(unuse, pdfDir, uv) = SampleLightDir(xi3);
    }
    if (pdfDir <= 0) {
      return {{}, {}};
    }
    Float theta = uv.y() * PI, phi = uv.x() * PI * 2;
    Vector3 d = Math::SphericalCoordinates(theta, phi);
    d = Vector3(d.y(), d.z(), -d.x());
    Vector3 worldD = _toWorld.ApplyAffineToWorld(-d);
    Vector3 perpendicularOffset = Frame(d).ToWorld(Vector3(offset.x(), offset.y(), 0));
    Vector3 origin = _worldSphere.Center + (perpendicularOffset - worldD) * _worldSphere.Radius;
    Ray ray{origin, worldD, 0, std::numeric_limits<Float>::max()};
    SurfaceInteraction q{};
    q.UV = uv;
    auto li = _map->Eval(q) * PI * Sqr(_worldSphere.Radius) / pdfDir;
    return std::make_pair(ray, Spectrum(li));
  }

  std::tuple<Ray, Spectrum, PositionSampleResult, Float, Float> SampleLe(
      const Vector2& xi2,
      const Vector2& xi3) const override {
    Vector2 offset = Warp::SquareToUniformDiskConcentric(xi2);
    Float pdfDir;
    Vector2 uv;
    if (_isUniformMap) {
      pdfDir = Warp::SquareToUniformSpherePdf();
      uv = xi3;
    } else {
      Vector3 unuse;
      std::tie(unuse, pdfDir, uv) = SampleLightDir(xi3);
    }
    if (pdfDir <= 0) {
      return {};
    }
    Float theta = uv.y() * PI, phi = uv.x() * PI * 2;
    Vector3 d = Math::SphericalCoordinates(theta, phi);
    d = Vector3(d.y(), d.z(), -d.x());
    Vector3 worldD = _toWorld.ApplyAffineToWorld(-d);
    Vector3 perpendicularOffset = Frame(d).ToWorld(Vector3(offset.x(), offset.y(), 0));
    Vector3 origin = _worldSphere.Center + (perpendicularOffset - worldD) * _worldSphere.Radius;
    Ray ray{origin, worldD, 0, std::numeric_limits<Float>::max()};
    SurfaceInteraction q{};
    q.UV = uv;
    Spectrum li = _map->Eval(q);
    Float pdfPos = 1 / (PI * Sqr(_worldSphere.Radius));
    PositionSampleResult psr{};
    psr.P = ray.O;
    psr.N = ray.D;
    psr.UV = uv;
    psr.Pdf = pdfPos;
    psr.IsDelta = false;
    return std::make_tuple(ray, li, psr, pdfPos, pdfDir);
  }

  std::pair<Float, Float> PdfLe(const PositionSampleResult& psr, const Vector3& dir) const override {
    DirectionSampleResult dsr{};
    dsr.Dir = dir;
    Float pdfDir = PdfDirection(Interaction{}, dsr);
    Float pdfPos = 1 / (PI * Sqr(_worldSphere.Radius));
    return std::make_pair(pdfPos, pdfDir);
  }

 private:
  std::tuple<Vector3, Float, Vector2> SampleLightDir(const Vector2& xi) const {
    auto [index, _, pdf] = _dist.Sample(xi);
    if (pdf <= 0) {
      return {{}, 0, {}};
    }
    Float y = index.y() / (Float)_map->Height();
    Float x = index.x() / (Float)_map->Width();
    Vector2 uv(x, y);
    Float theta = y * PI;
    Float phi = x * 2 * PI;
    Float sinTheta = std::sin(theta);
    if (sinTheta == 0) {
      return {{}, 0, {}};
    }
    pdf /= (2 * PI * PI * sinTheta);
    Vector3 sphDir = SphericalCoordinates(theta, phi);
    Vector3 dir(sphDir.y(), sphDir.z(), -sphDir.x());
    Vector3 worldDir = _toWorld.ApplyLinearToWorld(dir);
    return std::make_tuple(worldDir, pdf, uv);
  }

  Unique<TextureRGB> _map;
  Transform _toWorld;
  ContinuousDistribution2D _dist;
  bool _isUniformMap;
  BoundingSphere _worldSphere;
};

}  // namespace Rad

RAD_FACTORY_LIGHT_DECLARATION(Skybox, "skybox");
