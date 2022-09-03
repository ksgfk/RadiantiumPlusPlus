#include <radiantium/tracing_accel.h>

#include <radiantium/radiantium.h>
#include <spdlog/spdlog.h>
#include <embree3/rtcore.h>

namespace rad {

void EmbreeErrCallback(void* userPtr, enum RTCError error, const char* str);

class EmbreeAccel : public ITracingAccel {
 public:
  EmbreeAccel() {
    // _device = rtcNewDevice(NULL);
    // if (_device == nullptr) {
    //   RTCError err = rtcGetDeviceError(NULL);
    //   spdlog::info("embree error code {0}: cannot create device\n", static_cast<int>(err));
    //   return;
    // }
    // rtcSetDeviceErrorFunction(_device, EmbreeErrCallback, NULL);
    // _scene = rtcNewScene(_device);
  }

  ~EmbreeAccel() noexcept override {
    // if (_scene != nullptr) {
    //   rtcReleaseScene(_scene);
    //   _scene = nullptr;
    // }
    // if (_device != nullptr) {
    //   rtcReleaseDevice(_device);
    //   _device = nullptr;
    // }
  }

  bool IsValid() override { return _device != nullptr; }

  bool RayIntersect(Ray ray) override {
  }

 private:
  RTCDevice _device;
  RTCScene _scene;
};

void EmbreeErrCallback(void* userPtr, enum RTCError error, const char* str) {
  // spdlog::error("Embree log error: {0}, {1}", error, str);
}

}  // namespace rad
