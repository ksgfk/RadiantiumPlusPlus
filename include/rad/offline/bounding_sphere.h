#pragma once

#include "types.h"

namespace Rad {

/**
 * @brief 球形的包围球
 */
struct BoundingSphere {
  Vector3 Center;
  Float Radius;

  void Expand(const Vector3& p);
  bool Contains(const Vector3& p) const;
  
  static BoundingSphere FromBox(const BoundingBox3& box);
};

}  // namespace Rad
