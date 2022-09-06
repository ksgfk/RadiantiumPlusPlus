#include <radiantium/tracing_accel.h>

#include <radiantium/object.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>
#include <radiantium/radiantium.h>
#include <limits>
#include <stdexcept>
#include <utility>

namespace rad {

static void EmbreeErrCallback(void* userPtr, enum RTCError error, const char* str) {
  logger::GetLogger()->error("embree log error: {}, {}", error, str);
}

class EmbreeAccel : public ITracingAccel {
 public:
  EmbreeAccel() {
    _device = rtcNewDevice(NULL);
    if (_device == nullptr) {
      RTCError err = rtcGetDeviceError(NULL);
      logger::GetLogger()->error("embree error {}, cannot create device\n", err);
      return;
    }
    rtcSetDeviceErrorFunction(_device, EmbreeErrCallback, NULL);
    _scene = rtcNewScene(_device);
    rtcSetSceneBuildQuality(_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(_scene, RTC_SCENE_FLAG_NONE);
  }

  ~EmbreeAccel() noexcept override {
    if (_scene != nullptr) {
      rtcReleaseScene(_scene);
      _scene = nullptr;
    }
    if (_device != nullptr) {
      rtcReleaseDevice(_device);
      _device = nullptr;
    }
  }

  bool IsValid() override { return _device != nullptr; }

  const std::vector<IShape*> Shapes() noexcept override { return _shapes; }

  void Build(std::vector<IShape*>&& shapes) override {
    if (shapes.size() >= std::numeric_limits<UInt32>::max()) {
      throw std::invalid_argument("shape count out of max");
    }
    _shapes = std::move(shapes);
    for (size_t i = 0; i < _shapes.size(); i++) {
      _shapes[i]->SubmitToEmbree(_device, _scene, static_cast<UInt32>(i));
    }
    rtcCommitScene(_scene);
  }

  static struct RTCRay CvtRay(const Ray& ray) {
    struct RTCRay rtcray;
    rtcray.org_x = ray.O.x();
    rtcray.org_y = ray.O.y();
    rtcray.org_z = ray.O.z();
    rtcray.dir_x = ray.D.x();
    rtcray.dir_y = ray.D.y();
    rtcray.dir_z = ray.D.z();
    rtcray.tnear = ray.MinT;
    rtcray.tfar = ray.MaxT;
    rtcray.mask = 0;
    rtcray.flags = 0;
    return rtcray;
  }

  bool RayIntersect(const Ray& ray) override {
    struct RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    struct RTCRay rtcray = CvtRay(ray);
    rtcOccluded1(_scene, &context, &rtcray);
    return rtcray.tfar != ray.MaxT;
  }

  bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) override {
    struct RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    struct RTCRayHit rayhit;
    rayhit.ray = CvtRay(ray);
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcIntersect1(_scene, &context, &rayhit);
    HitShapeRecord rec{};
    bool anyHit;
    if (rayhit.ray.tfar != ray.MaxT) {  // hit
      uint32_t shapeIndex = rayhit.hit.geomID;
      uint32_t primIndex = rayhit.hit.primID;
      IShape* shape = _shapes[shapeIndex];
      rec.ShapeIndex = shapeIndex;
      rec.PrimitiveIndex = primIndex;
      rec.T = rayhit.ray.tfar;
      rec.PrimitiveUV = Vec2(rayhit.hit.u, rayhit.hit.v);
      rec.Shape = shape;
      rec.GeometryNormal = Vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
      anyHit = true;
    } else {
      anyHit = false;
    }
    hsr = rec;
    return anyHit;
  }

 private:
  RTCDevice _device;
  RTCScene _scene;
  std::vector<IShape*> _shapes;
};

}  // namespace rad

namespace rad::factory {

class EmbreeAccelFactory : public IFactory {
 public:
  ~EmbreeAccelFactory() noexcept override {}
  std::string UniqueId() const override { return "embree"; }
  std::unique_ptr<Object> Create(const BuildContext* context, const IConfigNode* config) const override {
    return std::make_unique<EmbreeAccel>();
  }
};
std::unique_ptr<IFactory> CreateEmbreeFactory() {
  return std::make_unique<EmbreeAccelFactory>();
}

}  // namespace rad::factory
