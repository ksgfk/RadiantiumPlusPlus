#include <radiantium/radiantium.h>
#include <radiantium/wavefront_obj_reader.h>
#include <radiantium/tracing_accel.h>
#include <radiantium/shape.h>
#include <radiantium/static_buffer.h>
#include <radiantium/spectrum.h>
#include <radiantium/camera.h>
#include <radiantium/image_utility.h>
#include <string>
#include <chrono>

int main(int argc, char** argv) {
  rad::logger::InitLogger();
  auto logger = rad::logger::GetLogger();
  std::filesystem::path a("C:\\Users\\ksgfk\\Desktop\\cube.obj");
  // std::string a("f 1/1         4/3 1/6");
  rad::WavefrontObjReader o(a);
  std::chrono::time_point<std::chrono::high_resolution_clock> st = std::chrono::high_resolution_clock::now();
  o.Read();
  std::chrono::time_point<std::chrono::high_resolution_clock> ed = std::chrono::high_resolution_clock::now();
  rad::Int64 r = std::chrono::duration_cast<std::chrono::milliseconds>(ed - st).count();
  rad::logger::GetLogger()->info("use time: {} ms", r);
  // rad::GetLogger()->info("face: {}, v: {}, n: {}, uv: {}");
  if (o.HasError()) {
    rad::logger::GetLogger()->error("objReader err: {}", o.Error());
  }
  rad::TriangleModel model = o.ToModel();

  std::unique_ptr<rad::ITracingAccel> accel = rad::CreateEmbree();
  std::unique_ptr<rad::IShape> mesh = rad::CreateMesh(model, {});
  accel->Build({mesh.get()});

  rad::UInt32 x = 256;
  rad::UInt32 y = 128;
  auto camera = rad::CreatePerspective(
      60,
      0.001f,
      15,
      rad::Vec3(5, 5, 5),
      rad::Vec3(0, 0, 0),
      rad::Vec3(0, 1, 0),
      {x, y});
  rad::StaticBuffer2D<rad::Spectrum>
      fb(x, y);
  for (rad::UInt32 j = 0; j < y; j++) {
    auto [ptr, len] = fb.GetRowSpan(j);
    for (rad::UInt32 i = 0; i < len; i++) {
      rad::Ray ray = camera->SampleRay({i, j});
      rad::HitShapeRecord rec;
      bool anyHit = accel->RayIntersectPreliminary(ray, rec);
      if (anyHit) {
        auto t = rec.ComputeSurfaceInteraction(ray);
        ptr[i] = rad::Spectrum(t.Shading.N);
      } else {
        ptr[i] = rad::Spectrum(0);
      }
    }
  }

  rad::image::SaveOpenExr("C:\\Users\\ksgfk\\Desktop\\test.exr", fb);

  rad::logger::ShutdownLogger();
}
