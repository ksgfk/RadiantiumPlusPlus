#include <radiantium/radiantium.h>
#include <radiantium/wavefront_obj_reader.h>
#include <radiantium/tracing_accel.h>
#include <radiantium/shape.h>
#include <string>
#include <chrono>

int main(int argc, char** argv) {
  rad::InitLogger();
  auto logger = rad::GetLogger();
  std::filesystem::path a("C:\\Users\\ksgfk\\Desktop\\cube.obj");
  // std::string a("f 1/1         4/3 1/6");
  rad::WavefrontObjReader o(a);
  std::chrono::time_point<std::chrono::high_resolution_clock> st = std::chrono::high_resolution_clock::now();
  o.Read();
  std::chrono::time_point<std::chrono::high_resolution_clock> ed = std::chrono::high_resolution_clock::now();
  rad::Int64 r = std::chrono::duration_cast<std::chrono::milliseconds>(ed - st).count();
  rad::GetLogger()->info("use time: {} ms", r);
  // rad::GetLogger()->info("face: {}, v: {}, n: {}, uv: {}");
  if (o.HasError()) {
    rad::GetLogger()->error("objReader err: {}", o.Error());
  }
  rad::TriangleModel model = o.ToModel();

  std::unique_ptr<rad::ITracingAccel> accel = rad::CreateEmbree();
  std::unique_ptr<rad::IShape> mesh = rad::CreateMesh(model, {});
  accel->Build({mesh.get()});

  rad::Ray ray{{0, 0, -3}, {0, 1, 0}, 0, 100000};
  bool anyHit = accel->RayIntersect(ray);
  logger->info("hit? {}", anyHit);

  rad::ShutdownLogger();
}
