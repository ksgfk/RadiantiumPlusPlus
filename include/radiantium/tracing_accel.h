#pragma once

#include "ray.h"
#include "interaction.h"
#include "shape.h"
#include "object.h"
#include "fwd.h"

namespace rad {

class ITracingAccel : public Object {
 public:
  virtual ~ITracingAccel() noexcept {}

  virtual bool IsValid() = 0;
  virtual const std::vector<IShape*> Shapes() noexcept = 0;

  virtual void Build(std::vector<IShape*>&& shapes) = 0;
  virtual bool RayIntersect(const Ray& ray) = 0;
  virtual bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateEmbreeFactory();
}

}  // namespace rad
