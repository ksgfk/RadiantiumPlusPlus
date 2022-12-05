#pragma once

#include "types.h"

namespace Rad {

/**
 * @brief 球形的包围球
 */
struct BoundingSphere {
  Vector3 Center;
  Float Radius;

  /**
   * @brief 将点p包含进球体
   */
  void Expand(const Vector3& p);
  /**
   * @brief 检查点p是否在球体内
   */
  bool Contains(const Vector3& p) const;

  /**
   * @brief 从轴对齐包围盒转换为外接球
   */
  static BoundingSphere FromBox(const BoundingBox3& box);
};

}  // namespace Rad
