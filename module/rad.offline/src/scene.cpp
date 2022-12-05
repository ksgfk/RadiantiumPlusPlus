#include <rad/offline/render/scene.h>

#include <rad/core/logger.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/render/accel.h>
#include <rad/offline/render/light.h>
#include <rad/offline/render/shape.h>

namespace Rad {

Scene::Scene(
    Unique<Accel> accel,
    Unique<Camera> camera,
    std::vector<Unique<Bsdf>>&& bsdfs,
    std::vector<Unique<Light>>&& lights,
    std::vector<Unique<Medium>>&& mediums,
    Medium* globalMedium)
    : _accel(std::move(accel)),
      _camera(std::move(camera)),
      _bsdfs(std::move(bsdfs)),
      _lights(std::move(lights)),
      _mediums(std::move(mediums)),
      _globalMedium(globalMedium) {
  _lightPdf = Math::Rcp(Float(_lights.size()));
  for (const auto& light : _lights) {
    if (light->IsEnv()) {
      if (_envLight != nullptr) {
        Logger::Get()->warn("multiple env light. only one valid");
      }
      _envLight = light.get();
      _envLight->SetScene(this);
    }
  }
  _lights.shrink_to_fit();
  _mediums.shrink_to_fit();
}

bool Scene::RayIntersect(const Ray& ray) const {
  return _accel->RayIntersect(ray);
}

bool Scene::RayIntersect(const Ray& ray, SurfaceInteraction& si) const {
  HitShapeRecord record;
  if (!_accel->RayIntersectPreliminary(ray, record)) {
    si = {};
    si.Wi = -ray.D;
    return false;
  }
  si = record.ComputeSurfaceInteraction(ray);
  return true;
}

bool Scene::IsOcclude(const Interaction& ref, const Vector3& p) const {
  Vector3 o = ref.OffsetP(p - ref.P);
  Vector3 d = p - o;
  Float dist = d.norm();
  if (dist < Math::ShadowEpsilon) {
    return true;
  }
  d /= dist;
  Ray shadowRay{o, d, 0, dist * (1 - Math::ShadowEpsilon)};
  return RayIntersect(shadowRay);
}

std::pair<UInt32, Float> Scene::SampleLight(Float xi) const {
  if (_lights.size() < 2) {
    if (_lights.size() == 1) {
      return std::make_pair(UInt32(0), Float(1));
    } else {
      return std::make_pair(UInt32(-1), Float(0));
    }
  }
  uint32_t lightCount = (uint32_t)_lights.size();
  Float lightIndex = xi * lightCount;
  UInt32 index = std::min(UInt32(lightIndex), lightCount - 1);
  return std::make_pair(index, _lightPdf);
}

Float Scene::PdfLight(UInt32 index) const {
  return _lightPdf;
}

Float Scene::PdfLight(const Light* light) const {
  return _lightPdf;
}

std::tuple<Light*, DirectionSampleResult, Spectrum> Scene::SampleLightDirection(
    const Interaction& ref,
    Float sampleLight,
    const Vector2& xi) const {
  auto [index, pdf] = SampleLight(sampleLight);
  Light* light = _lights[index].get();
  auto [dsr, li] = light->SampleDirection(ref, xi);
  dsr.Pdf *= pdf;
  return std::make_tuple(light, dsr, li);
}

Float Scene::PdfLightDirection(
    const Light* light,
    const Interaction& ref,
    const DirectionSampleResult& dsr) const {
  return light->PdfDirection(ref, dsr) * _lightPdf;
}

std::optional<Light*> Scene::GetLight(const SurfaceInteraction& si) const {
  return (si.Shape == nullptr)
             ? (_envLight == nullptr
                    ? std::nullopt
                    : std::make_optional(_envLight))
             : (si.Shape->IsLight()
                    ? std::make_optional(si.Shape->GetLight())
                    : std::nullopt);
}

Spectrum Scene::EvalLight(const SurfaceInteraction& si) const {
  std::optional<Light*> light = GetLight(si);
  return light.has_value() ? (*light)->Eval(si) : Spectrum(0);
}

BoundingBox3 Scene::GetWorldBound() const {
  return _accel->GetWorldBound();
}

}  // namespace Rad
