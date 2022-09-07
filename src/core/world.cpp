#include <radiantium/world.h>

#include <radiantium/camera.h>
#include <radiantium/sampler.h>
#include <radiantium/tracing_accel.h>

namespace rad {

ISampler* World::GetSampler() const { return _sampler.get(); }

const ICamera* World::GetCamera() const { return _camera.get(); }

bool World::RayIntersect(const Ray& ray) const {
  return _accel->RayIntersect(ray);
}

bool World::RayIntersect(const Ray& ray, SurfaceInteraction& si) const {
  HitShapeRecord record;
  if (!_accel->RayIntersectPreliminary(ray, record)) {
    return false;
  }
  si = record.ComputeSurfaceInteraction(ray);
  return true;
}

}  // namespace rad
