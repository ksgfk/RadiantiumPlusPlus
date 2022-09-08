#pragma once

#include "radiantium.h"

namespace rad {
/**
 * @brief 射线
 */
struct Ray {
  /**
   * @brief 射线起点
   */
  Vec3 O;
  /**
   * @brief 射线方向
   */
  Vec3 D;
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
  Vec3 operator()(Float t) const;
};

}  // namespace rad
