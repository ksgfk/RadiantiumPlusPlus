#pragma once

#include "ray.h"

namespace rad {

class ITracingAccel {
 public:
  virtual ~ITracingAccel() noexcept;

  virtual bool IsValid() = 0;
  virtual bool RayIntersect(Ray ray) = 0;
};

}  // namespace rad
