#pragma once

#include "../common.h"
#include "interaction.h"

#include <vector>

namespace Rad {

class Scene {
 public:
  Scene(Unique<Accel> accel, Unique<Camera> camera, std::vector<Unique<Light>>&& lights);

  const Camera& GetCamera() const { return *_camera; }
  Camera& GetCamera() { return *_camera; }
  const Light* GetLight(UInt32 index) const { return _lights[index].get(); }
  Light* GetLight(UInt32 index) { return _lights[index].get(); }

  bool RayIntersect(const Ray& ray) const;
  bool RayIntersect(const Ray& ray, SurfaceInteraction& si) const;
  bool IsOcclude(const Interaction& ref, const Vector3& p) const;

  std::tuple<UInt32, Float, Float> SampleLight(Float xi) const;
  Float PdfLight(UInt32 index) const;
  std::tuple<Light*, DirectionSampleResult, Spectrum> SampleLightDirection(
      const Interaction& ref,
      Float sampleLight,
      const Vector2& xi) const;

 private:
  Unique<Accel> _accel;
  Unique<Camera> _camera;
  std::vector<Unique<Light>> _lights;
  Float _lightPdf;
};

}  // namespace Rad
