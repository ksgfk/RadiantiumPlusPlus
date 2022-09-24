#include <rad/offline/render/accel.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

namespace Rad {

static Share<spdlog::logger> _logger;

static void EmbreeErrCallback(void* userPtr, enum RTCError error, const char* str) {
  _logger->error("embree log error: {}, {}", error, str);
}

class Embree final : public Accel {
 public:
  Embree(BuildContext* ctx, const ConfigNode& cfg) {
    _shapes = std::move(ctx->GetShapes());
    // init
    _logger = Logger::GetCategory("embree");
    _device = rtcNewDevice(NULL);
    if (_device == nullptr) {
      RTCError err = rtcGetDeviceError(NULL);
      throw RadInvalidOperationException("embree error {}, cannot create device", err);
    }
    rtcSetDeviceErrorFunction(_device, EmbreeErrCallback, NULL);
    _scene = rtcNewScene(_device);
    rtcSetSceneBuildQuality(_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(_scene, RTC_SCENE_FLAG_NONE);
    // build
    if (_shapes.size() >= std::numeric_limits<UInt32>::max()) {
      throw RadArgumentException("shape count out of max");
    }
    for (size_t i = 0; i < _shapes.size(); i++) {
      _shapes[i]->SubmitToEmbree(_device, _scene, static_cast<UInt32>(i));
    }
    rtcCommitScene(_scene);
  }
  ~Embree() noexcept override {
    if (_scene != nullptr) {
      rtcReleaseScene(_scene);
      _scene = nullptr;
    }
    if (_device != nullptr) {
      rtcReleaseDevice(_device);
      _device = nullptr;
    }
  }

  static struct RTCRay ToEmbreeRay(const Ray& ray) {
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
    struct RTCRay rtcray = ToEmbreeRay(ray);
    rtcOccluded1(_scene, &context, &rtcray);
    return rtcray.tfar != ray.MaxT;
  }

  bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) override {
    struct RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    struct RTCRayHit rayhit;
    rayhit.ray = ToEmbreeRay(ray);
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcIntersect1(_scene, &context, &rayhit);
    HitShapeRecord rec{};
    bool anyHit;
    if (rayhit.ray.tfar != ray.MaxT) {  // hit
      uint32_t shapeIndex = rayhit.hit.geomID;
      uint32_t primIndex = rayhit.hit.primID;
      rec.ShapePtr = _shapes[shapeIndex].get();
      rec.PrimitiveIndex = primIndex;
      rec.T = rayhit.ray.tfar;
      rec.PrimitiveUV = Vector2(rayhit.hit.u, rayhit.hit.v);
      rec.ShapeIndex = shapeIndex;
      rec.GeometryNormal = Vector3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
      anyHit = true;
    } else {
      anyHit = false;
    }
    hsr = rec;
    return anyHit;
  }

 private:
  std::vector<Unique<Shape>> _shapes;
  RTCDevice _device;
  RTCScene _scene;
};

}  // namespace Rad

RAD_FACTORY_ACCEL_DECLARATION(Embree, "embree");