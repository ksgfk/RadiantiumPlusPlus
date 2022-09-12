#include <radiantium/camera.h>

#include <radiantium/object.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

#include <radiantium/transform.h>
#include <radiantium/math_ext.h>
#include <cmath>

namespace rad {

class Perspective : public ICamera {
 public:
  Perspective(Float fov, Float near, Float far,
              const Vec3& origin, const Vec3& target, const Vec3& up,
              const Eigen::Vector2i& resolution)
      : Perspective(fov, near, far, math::LookAtLeftHand(origin, target, up), resolution) {}
  Perspective(Float fov, Float near, Float far, const Mat4& cameraToWorld, const Eigen::Vector2i& resolution) {
    _near = near;
    _far = far;
    _resolution = resolution;
    _rcpResolution = Vec2(Float(_resolution.x()), Float(_resolution.y())).cwiseInverse();
    _cameraToWorld = Transform(cameraToWorld);

    Float aspect = _resolution.x() / static_cast<Float>(_resolution.y());
    Float recip = 1 / (far - near);                   //将相机空间中的向量投影到z=1的平面上
    Float cot = 1 / std::tan(math::Radian(fov / 2));  // cotangent确保NDC的near到far是[0,1]
    Mat4 perspective;
    perspective << cot, 0, 0, 0,
        0, cot, 0, 0,
        0, 0, far * recip, -near * far * recip,
        0, 0, 1, 0;
    Mat4 scale;
    scale << -Float(0.5), 0, 0, 0,
        0, -Float(0.5) * aspect, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;
    Mat4 translate;
    translate << 1, 0, 0, -1,
        0, 1, 0, -1 / aspect,
        0, 0, 1, 0,
        0, 0, 0, 1;
    Mat4 toClip = scale * translate * perspective;
    _cameraToClip = Transform(toClip);
  }
  ~Perspective() noexcept override {}

  Eigen::Vector2i Resolution() const override { return _resolution; }

  Vec3 WorldPosition() const override { return _cameraToWorld.ApplyAffineToWorld(Vec3(0, 0, 0)); }

  Ray SampleRay(Vec2 screenPosition) const override {
    Vec2 t = screenPosition.cwiseProduct(_rcpResolution);
    Vec3 ndc = Vec3(t.x(), t.y(), 0);
    Vec3 near = _cameraToClip.ApplyAffineToLocal(ndc);  //标准设备空间的近平面变换到摄像机空间
    Vec3 dir = near.normalized();
    Float invZ = math::Rcp(dir.z());
    Vec3 o = _cameraToWorld.ApplyAffineToWorld(Vec3(0, 0, 0));  //从摄像机空间变换到世界空间
    Vec3 d = _cameraToWorld.ApplyLinearToWorld(dir).normalized();
    return Ray{o, d, _near * invZ, _far * invZ};
  }

 private:
  Float _near;
  Float _far;
  Eigen::Vector2i _resolution;
  Vec2 _rcpResolution;
  Transform _cameraToWorld;
  Transform _cameraToClip;
};

}  // namespace rad

namespace rad::factory {

class PerspectiveCameraFactory : public ICameraFactory {
 public:
  ~PerspectiveCameraFactory() noexcept override {}
  std::string UniqueId() const override { return "perspective"; }
  std::unique_ptr<ICamera> Create(const BuildContext* context, const IConfigNode* config) const override {
    std::unique_ptr<Perspective> result;
    Float fov = config->GetFloat("fov", 30);
    Float near = config->GetFloat("near", 0.001f);
    Float far = config->GetFloat("far", 100);
    Vec2 reso = config->GetVec2("resolution", Vec2(1280, 720));
    Eigen::Vector2i resolution{(Int32)reso.x(), (Int32)reso.y()};
    if (config->HasProperty("to_world")) {
      Mat4 toWorld = config->GetMat4("to_world", Mat4::Identity());
      result = std::make_unique<Perspective>(fov, near, far, toWorld, resolution);
    } else {
      Vec3 origin = config->GetVec3("origin", Vec3(0, 0, -1));
      Vec3 target = config->GetVec3("target", Vec3(0, 0, 0));
      Vec3 up = config->GetVec3("up", Vec3(0, 1, 0));
      result = std::make_unique<Perspective>(fov, near, far, origin, target, up, resolution);
    }
    return result;
  }
};
std::unique_ptr<IFactory> CreatePerspectiveFactory() {
  return std::make_unique<PerspectiveCameraFactory>();
}

}  // namespace rad::factory
