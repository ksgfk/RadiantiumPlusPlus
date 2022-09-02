#include <iostream>

#include <spdlog/spdlog.h>
#include <embree3/rtcore.h>

#include <radiantium/static_buffer.h>
#include <radiantium/spectrum.h>
#include <radiantium/image_utility.h>
#include <type_traits>
#include <random>

int main(int argc, char** argv) {
  spdlog::info("Spectrum size {0}", sizeof(rad::Spectrum));
  spdlog::info("Spectrum is standard layout {0}", std::is_standard_layout<rad::Spectrum>::value);

  std::mt19937 rnd;
  std::uniform_real_distribution<float> dist;

  rad::StaticBuffer<rad::Spectrum> a(256, 128);
  for (uint32_t y = 0; y < a.Height(); y++) {
    for (uint32_t x = 0; x < a.Width(); x++) {
      a(x,y) = rad::Spectrum(dist(rnd), dist(rnd), dist(rnd));
    }
  }
  
  rad::SaveOpenExr("C:\\Users\\ksgfk\\Desktop\\test.exr", a);

  return 0;

  // rad::StaticBuffer<int> a(4, 4);
  // uint32_t i = a.GetIndex(0, 1);
  // a(3, 1) = 1;
  // auto [addr, count] = a.GetRowSpan(1);
  // for (size_t i = 0; i < count; i++) {
  //   spdlog::info("{0}", addr[i]);
  // }

  // std::cout << "hello mtfk?" << std::endl;

  // spdlog::info("Welcome to spdlog!");
  // spdlog::error("Some error message with arg: {}", 1);
  // spdlog::warn("Easy padding in numbers like {:08d}", 12);
  // spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  // spdlog::info("Support for floats {:03.2f}", 1.23456);
  // spdlog::info("Positional args are {1} {0}..", "too", "supported");
  // spdlog::info("{:<30}", "left aligned");

  // RTCDevice device = rtcNewDevice(NULL);
  // if (!device)
  //   spdlog::info("error {0}: cannot create device\n", rtcGetDeviceError(NULL));
  // rtcSetDeviceErrorFunction(device, errorFunction, NULL);
  // RTCScene scene = rtcNewScene(device);
  // RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
  // float* vertices = (float*)rtcSetNewGeometryBuffer(geom,
  //                                                   RTC_BUFFER_TYPE_VERTEX,
  //                                                   0,
  //                                                   RTC_FORMAT_FLOAT3,
  //                                                   3 * sizeof(float),
  //                                                   3);

  // unsigned* indices = (unsigned*)rtcSetNewGeometryBuffer(geom,
  //                                                        RTC_BUFFER_TYPE_INDEX,
  //                                                        0,
  //                                                        RTC_FORMAT_UINT3,
  //                                                        3 * sizeof(unsigned),
  //                                                        1);
  // if (vertices && indices) {
  //   vertices[0] = 0.f;
  //   vertices[1] = 0.f;
  //   vertices[2] = 0.f;
  //   vertices[3] = 1.f;
  //   vertices[4] = 0.f;
  //   vertices[5] = 0.f;
  //   vertices[6] = 0.f;
  //   vertices[7] = 1.f;
  //   vertices[8] = 0.f;

  //   indices[0] = 0;
  //   indices[1] = 1;
  //   indices[2] = 2;
  // }
  // rtcCommitGeometry(geom);
  // uint32_t geoId = rtcAttachGeometry(scene, geom);
  // rtcReleaseGeometry(geom);
  // rtcCommitScene(scene);

  // castRay(scene, 0, 0, -1, 0, 0, 1);
  // castRay(scene, 1, 1, -1, 0, 0, 1);

  // rtcReleaseScene(scene);
  // rtcReleaseDevice(device);

  // return 0;
}

void errorFunction(void* userPtr, enum RTCError error, const char* str) {
  // spdlog::error("Embree log error: {0}, {1}", error, str);
}

void castRay(RTCScene scene,
             float ox, float oy, float oz,
             float dx, float dy, float dz) {
  // /*
  //  * The intersect context can be used to set intersection
  //  * filters or flags, and it also contains the instance ID stack
  //  * used in multi-level instancing.
  //  */
  // struct RTCIntersectContext context;
  // rtcInitIntersectContext(&context);

  // /*
  //  * The ray hit structure holds both the ray and the hit.
  //  * The user must initialize it properly -- see API documentation
  //  * for rtcIntersect1() for details.
  //  */
  // struct RTCRayHit rayhit;
  // rayhit.ray.org_x = ox;
  // rayhit.ray.org_y = oy;
  // rayhit.ray.org_z = oz;
  // rayhit.ray.dir_x = dx;
  // rayhit.ray.dir_y = dy;
  // rayhit.ray.dir_z = dz;
  // rayhit.ray.tnear = 0;
  // rayhit.ray.tfar = std::numeric_limits<float>::infinity();
  // rayhit.ray.mask = -1;
  // rayhit.ray.flags = 0;
  // rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  // rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

  // /*
  //  * There are multiple variants of rtcIntersect. This one
  //  * intersects a single ray with the scene.
  //  */
  // rtcIntersect1(scene, &context, &rayhit);

  // spdlog::info("{0}, {1}, {2}: ", ox, oy, oz);
  // if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
  //   /* Note how geomID and primID identify the geometry we just hit.
  //    * We could use them here to interpolate geometry information,
  //    * compute shading, etc.
  //    * Since there is only a single triangle in this scene, we will
  //    * get geomID=0 / primID=0 for all hits.
  //    * There is also instID, used for instancing. See
  //    * the instancing tutorials for more information */
  //   spdlog::info("Found intersection on geometry {0}, primitive {1} at tfar={2}",
  //                rayhit.hit.geomID,
  //                rayhit.hit.primID,
  //                rayhit.ray.tfar);
  // } else
  //   spdlog::info("Did not find any intersection.");
}
