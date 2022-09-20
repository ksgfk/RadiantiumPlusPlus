#pragma once

#include "types.h"

namespace Rad {
/**
 * @brief 射线
 */
struct Ray {
  /**
   * @brief 射线起点
   */
  Vector3 O;
  /**
   * @brief 射线方向
   */
  Vector3 D;
  /**
   * @brief 最小距离
   */
  Float MinT;
  /**
   * @brief 最大距离
   */
  Float MaxT;
  /**
   * @brief 根据距离计算光线到达的坐标
   */
  inline Vector3 operator()(Float t) const { return D * t + O; }
};

}  // namespace Rad
