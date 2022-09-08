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

 private:
  std::unique_ptr<IShape> _shape;

  friend class BuildContext;
};

class World {
 public:
  ISampler* GetSampler() const;
  const ICamera* GetCamera() const;

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
