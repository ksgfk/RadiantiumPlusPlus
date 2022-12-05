#include <rad/offline/render/camera.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/transform.h>

namespace Rad {

/**
 * @brief 理想透视相机, 它有一个无限小的光圈, 因此不会有景深效果
 */
class Perspective final : public Camera {
 public:
  Perspective(BuildContext* ctx, const ConfigNode& cfg) {
    _near = cfg.ReadOrDefault("near", Float(0.001));
    _far = cfg.ReadOrDefault("far", Float(100));
    Float fov = cfg.ReadOrDefault("fov", Float(30));
    _resolution = cfg.ReadOrDefault("resolution", Eigen::Vector2i(1280, 720));
    Matrix4 toWorld;
    ConfigNode toWorldNode;
    if (!cfg.TryRead("to_world", toWorldNode)) {
      Vector3 origin = cfg.ReadOrDefault("origin", Vector3(0, 0, -1));
      Vector3 target = cfg.ReadOrDefault("target", Vector3(0, 0, 0));
      Vector3 up = cfg.ReadOrDefault("up", Vector3(0, 1, 0));
      toWorld = Math::LookAtLeftHand(origin, target, up);
    } else {
      toWorld =ConfigNodeAsTransform(toWorldNode);
    }

    _rcpResolution = _resolution.cast<Float>().cwiseInverse();
    _cameraToWorld = Transform(toWorld);
    Float aspect = _resolution.x() / static_cast<Float>(_resolution.y());
    Float recip = 1 / (_far - _near);                 //将相机空间中的向量投影到z=1的平面上
    Float cot = 1 / std::tan(Math::Radian(fov / 2));  // cotangent确保NDC的near到far是[0,1]
    Matrix4 perspective;
    perspective << cot, 0, 0, 0,
        0, cot, 0, 0,
        0, 0, _far * recip, -_near * _far * recip,
        0, 0, 1, 0;
    Matrix4 scale;
    scale << -Float(0.5), 0, 0, 0,
        0, -Float(0.5) * aspect, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;
    Matrix4 translate;
    translate << 1, 0, 0, -1,
        0, 1, 0, -1 / aspect,
        0, 0, 1, 0,
        0, 0, 0, 1;
    Matrix4 toClip = scale * translate * perspective;
    _cameraToClip = Transform(toClip);

    _position = _cameraToWorld.ApplyAffineToWorld(Vector3(0, 0, 0));

    _dx = _cameraToClip.ApplyAffineToLocal(Vector3(Float(1) / _resolution.x(), 0, 0)) -
          _cameraToClip.ApplyAffineToLocal(Vector3::Constant(0));
    _dy = _cameraToClip.ApplyAffineToLocal(Vector3(0, Float(1) / _resolution.y(), 0)) -
          _cameraToClip.ApplyAffineToLocal(Vector3::Constant(0));

    Vector3 pMin = _cameraToClip.ApplyAffineToLocal(Vector3(0, 0, 0));
    Vector3 pMax = _cameraToClip.ApplyAffineToLocal(Vector3(1, 1, 0));
    _imageRect.extend(pMin.head<2>() / pMin.z());
    _imageRect.extend(pMax.head<2>() / pMax.z());
    _normalization = 1 / _imageRect.volume();

    _imagePlaneDist = _resolution.x() * cot * Float(0.5);

    InitCamera();
  }
  ~Perspective() noexcept override = default;

  Ray SampleRay(const Vector2& screenPosition) const override {
    Vector2 t = screenPosition.cwiseProduct(_rcpResolution);
    Vector3 ndc = Vector3(t.x(), t.y(), 0);
    Vector3 near = _cameraToClip.ApplyAffineToLocal(ndc);  //标准设备空间的近平面变换到摄像机空间
    Vector3 dir = near.normalized();
    Float invZ = Math::Rcp(dir.z());
    Vector3 o = _cameraToWorld.ApplyAffineToWorld(Vector3(0, 0, 0));  //从摄像机空间变换到世界空间
    Vector3 d = _cameraToWorld.ApplyLinearToWorld(dir).normalized();
    return Ray{o, d, _near * invZ, _far * invZ};
  }

  RayDifferential SampleRayDifferential(const Vector2& screenPosition) const override {
    Vector2 t = screenPosition.cwiseProduct(_rcpResolution);
    Vector3 ndc = Vector3(t.x(), t.y(), 0);
    Vector3 near = _cameraToClip.ApplyAffineToLocal(ndc);  //标准设备空间的近平面变换到摄像机空间
    Vector3 dir = near.normalized();
    Float invZ = Math::Rcp(dir.z());
    Vector3 o = _cameraToWorld.ApplyAffineToWorld(Vector3(0, 0, 0));  //从摄像机空间变换到世界空间
    Vector3 d = _cameraToWorld.ApplyLinearToWorld(dir).normalized();
    RayDifferential diff{
        {o, d, _near * invZ, _far * invZ},
        o,
        o,
        _cameraToWorld.ApplyLinearToWorld((near + _dx).normalized()),
        _cameraToWorld.ApplyLinearToWorld((near + _dy).normalized())};
    return diff;
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    Vector3 refP = _cameraToWorld.ApplyAffineToLocal(ref.P);
    DirectionSampleResult dsr{};
    dsr.Pdf = 0;
    if (refP.z() < _near || refP.z() > _far) {
      return {dsr, Spectrum(0)};
    }
    Vector3 ndcPos = _cameraToClip.ApplyAffineToWorld(refP);
    if (ndcPos.x() < 0 || ndcPos.x() >= 1 || ndcPos.y() < 0 || ndcPos.y() >= 1) {
      return {dsr, Spectrum(0)};
    }
    dsr.UV = ndcPos.head<2>().cwiseProduct(_resolution.cast<Float>());
    Float dist = refP.norm();
    Float invDist = Math::Rcp(dist);
    Vector3 dir = refP * invDist;
    dsr.P = _cameraToWorld.ApplyAffineToWorld(Vector3::Zero());
    dsr.Dir = (dsr.P - ref.P) * invDist;
    dsr.Dist = dist;
    dsr.N = _cameraToWorld.ApplyLinearToWorld(Vector3(0, 0, 1));
    dsr.Pdf = 1;
    Float importance = Importance(dir) * invDist * invDist;
    return std::make_pair(dsr, Spectrum(importance));
  }

  Float Importance(const Vector3& d) const {
    Float cosTheta = Frame::CosTheta(d);
    if (cosTheta <= 0) {
      return 0;
    }
    Float invCosTheta = Math::Rcp(cosTheta);
    Vector2 p = d.head<2>() * invCosTheta;
    if (!_imageRect.contains(p)) {
      return 0;
    }
    Float importance = _normalization * invCosTheta * invCosTheta * invCosTheta;
    return importance;
  }

  std::pair<Float, Float> PdfWe(const Ray& ray) const override {
    Vector3 dir = _cameraToWorld.ApplyLinearToLocal(ray.D);
    Float pdfPos = 1;
    Float pdfDir = Importance(dir);
    return std::make_pair(pdfPos, pdfDir);
  }

 private:
  Float _near;
  Float _far;
  Vector2 _rcpResolution;
  Transform _cameraToWorld;
  Transform _cameraToClip;
  Vector3 _dx, _dy;
  BoundingBox2 _imageRect;
  Float _normalization;
  Float _imagePlaneDist;
};

class PerspectiveFactory final : public CameraFactory {
 public:
  PerspectiveFactory() : CameraFactory("perspective") {}
  ~PerspectiveFactory() noexcept override = default;
  Unique<Camera> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Perspective>(ctx, cfg);
  }
};

Unique<CameraFactory> _FactoryCreatePerspectiveFunc_() {
  return std::make_unique<PerspectiveFactory>();
}

}  // namespace Rad
