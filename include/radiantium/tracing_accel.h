#pragma once

#include "ray.h"
#include "interaction.h"
#include "shape.h"

namespace rad {

class ITracingAccel {
 public:
  virtual ~ITracingAccel() noexcept = default;

  virtual bool IsValid() = 0;
  virtual const std::vector<IShape*> Shapes() noexcept = 0;

  virtual void Build(std::vector<IShape*>&& shapes) = 0;
  virtual bool RayIntersect(const Ray& ray) = 0;
  virtual bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) = 0;
};

std::unique_ptr<ITracingAccel> CreateEmbree();

}  // namespace rad
