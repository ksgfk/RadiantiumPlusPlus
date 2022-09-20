#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

class Accel {
 public:
  virtual ~Accel() noexcept = default;

  /**
   * @brief shadow ray
   */
  virtual bool RayIntersect(const Ray& ray) = 0;
  /**
   * @brief 初步求交
   *
   * @param hsr out 返回求交结果
   */
  virtual bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) = 0;
};

}  // namespace Rad
