#pragma once

#include "types.h"

namespace Rad {
/**
 * @brief 光线
 */
struct Ray {
  /**
   * @brief 光线起点
   */
  Vector3 O;
  /**
   * @brief 光线方向
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

/**
 * @brief 光线微分, 包含两条辅助光线, 这两条光线是采样时在x与y方向上偏移一个采样单位
 */
struct RayDifferential : Ray {
  Vector3 Ox, Oy;
  Vector3 Dx, Dy;
  bool HasDifferentials = false;

  RayDifferential() = default;
  RayDifferential(const Ray& ray) : Ray(ray) { HasDifferentials = false; }
  RayDifferential(
      const Ray& ray,
      const Vector3& ox, const Vector3& oy,
      const Vector3& dx, const Vector3& dy)
      : Ray(ray), Ox(ox), Oy(oy), Dx(dx), Dy(dy) { HasDifferentials = true; }

  /**
   * @brief 用于更新差分光线
   */
  inline void ScaleDifferential(Float amount) {
    Ox = (Ox - O) * amount + O;
    Oy = (Oy - O) * amount + O;
    Dx = (Dx - D) * amount + D;
    Dy = (Dx - D) * amount + D;
  }
};

}  // namespace Rad
