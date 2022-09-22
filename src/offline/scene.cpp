#include <rad/offline/render/scene.h>

#include <rad/offline/render.h>

namespace Rad {

Scene::Scene(Unique<Accel> accel, Unique<Camera> camera, std::vector<Unique<Light>>&& lights)
    : _accel(std::move(accel)),
      _camera(std::move(camera)),
      _lights(std::move(lights)) {
  _lightPdf = Math::Rcp(Float(_lights.size()));
}

bool Scene::RayIntersect(const Ray& ray) const {
  return _accel->RayIntersect(ray);
}

bool Scene::RayIntersect(const Ray& ray, SurfaceInteraction& si) const {
  HitShapeRecord record;
  if (!_accel->RayIntersectPreliminary(ray, record)) {
    return false;
  }
  si = record.ComputeSurfaceInteraction(ray);
  return true;
}

bool Scene::IsOcclude(const Interaction& ref, const Vector3& p) const {
  Ray ray = ref.SpawnRayTo(p);
  return RayIntersect(ray);
}

std::tuple<UInt32, Float, Float> Scene::SampleLight(Float xi) const {
  if (_lights.size() < 2) {
    if (_lights.size() == 1) {
      return std::make_tuple(UInt32(0), Float(1), xi);
    } else {
      return std::make_tuple(UInt32(-1), Float(0), xi);
    }
  }
  uint32_t lightCount = (uint32_t)_lights.size();
  Float lcf = (Float)lightCount;
  Float lightIndex = xi * lcf;
  UInt32 index = std::min(UInt32(lightIndex), lightCount - 1);
  return std::make_tuple(index, lcf, lightIndex - Float(index));
}

Float Scene::PdfLight(UInt32 index) const {
  return _lightPdf;
}

std::tuple<Light*, DirectionSampleResult, Spectrum> Scene::SampleLightDirection(
    const Interaction& ref,
    Float sampleLight,
    const Vector2& xi) const {
  auto [index, weight, sampleXReuse] = SampleLight(sampleLight);
  Light* light = _lights[index].get();
  auto [dsr, li] = light->SampleDirection(ref, xi);
  dsr.Pdf /= weight;
  return std::make_tuple(light, dsr, li);
}

}  // namespace Rad
