#pragma once

#include <rad/offline/types.h>

namespace Rad {

struct PositionSampleResult {
  /**
   * @brief 采样点坐标
   */
  Vector3 P;
  /**
   * @brief 采样点坐标的法线方向
   */
  Vector3 N;
  /**
   * @brief 纹理坐标, 这个参数不是必须的
   */
  Vector2 UV;
  /**
   * @brief 样本的概率密度
   */
  Float Pdf;
  /**
   * @brief 样本是不是delta分布
   */
  bool IsDelta;
};

struct DirectionSampleResult : PositionSampleResult {
  /**
   * @brief 采样方向
   */
  Vector3 Dir;
  /**
   * @brief 从参考点到采样点的距离
   */
  Float Dist;
};

}  // namespace Rad
