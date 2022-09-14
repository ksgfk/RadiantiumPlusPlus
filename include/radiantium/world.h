#pragma once

#include <memory>
#include <vector>

#include "fwd.h"
#include "ray.h"
#include "interaction.h"
#include "sampler.h"
#include "camera.h"
#include "tracing_accel.h"

namespace rad {

class Entity {
 public:
  bool HasShape() const { return _shape != nullptr; }
  IShape* Shape() const { return _shape.get(); }

  bool HasBsdf() const { return _bsdf != nullptr; }
  IBsdf* Bsdf() const { return _bsdf.get(); }

  bool HasLight() const { return _light != nullptr; }
  ILight* Light() const { return _light.get(); }

 private:
  std::unique_ptr<IShape> _shape;
  std::unique_ptr<IBsdf> _bsdf;
  std::unique_ptr<ILight> _light;

  friend class BuildContext;
};

class World {
 public:
  ISampler* GetSampler() const;
  const ICamera* GetCamera() const;
  const Entity& GetEntity(UInt32 entityIndex) const;

  /**
   * @brief shadow ray
   */
  bool RayIntersect(const Ray& ray) const;
  /**
   * @brief 光线求交, 忽略参与介质
   */
  bool RayIntersect(const Ray& ray, SurfaceInteraction& si) const;

 private:
  std::vector<Entity> _entity;
  std::unique_ptr<ITracingAccel> _accel;
  std::unique_ptr<ISampler> _sampler;
  std::unique_ptr<ICamera> _camera;

  friend class BuildContext;
};

}  // namespace rad
