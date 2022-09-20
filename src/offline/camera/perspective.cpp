#include <rad/offline/render/camera.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

namespace Rad {

class Perspective final : public Camera {
 public:
  Perspective(BuildContext* ctx, const ConfigNode& cfg) {
    _near = cfg.ReadOrDefault("near", Float(0.001));
    _far = cfg.ReadOrDefault("far", Float(100));
    Float fov = cfg.ReadOrDefault("fov", Float(30));
    _resolution = cfg.ReadOrDefault("resolution", Eigen::Vector2i(1280, 720));
    Matrix4 toWorld;
    if (!cfg.TryRead("to_world", toWorld)) {
      Vector3 origin = cfg.ReadOrDefault("origin", Vector3(0, 0, -1));
      Vector3 target = cfg.ReadOrDefault("target", Vector3(0, 0, 0));
      Vector3 up = cfg.ReadOrDefault("up", Vector3(0, 1, 0));
      toWorld = Math::LookAtLeftHand(origin, target, up);
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

    InitCamera();
  }
  ~Perspective() noexcept override = default;

  Ray SampleRay(Vector2 screenPosition) const override {
    Vector2 t = screenPosition.cwiseProduct(_rcpResolution);
    Vector3 ndc = Vector3(t.x(), t.y(), 0);
    Vector3 near = _cameraToClip.ApplyAffineToLocal(ndc);  //标准设备空间的近平面变换到摄像机空间
    Vector3 dir = near.normalized();
    Float invZ = Math::Rcp(dir.z());
    Vector3 o = _cameraToWorld.ApplyAffineToWorld(Vector3(0, 0, 0));  //从摄像机空间变换到世界空间
    Vector3 d = _cameraToWorld.ApplyLinearToWorld(dir).normalized();
    return Ray{o, d, _near * invZ, _far * invZ};
  }

 private:
  Float _near;
  Float _far;
  Vector2 _rcpResolution;
  Transform _cameraToWorld;
  Transform _cameraToClip;
};

}  // namespace Rad

RAD_FACTORY_CAMERA_DECLARATION(Perspective, "perspective");
