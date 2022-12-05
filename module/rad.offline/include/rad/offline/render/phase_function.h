#pragma once

#include "interaction.h"

namespace Rad {

/**
 * @brief 相函数描述了光线在参与介质中散射时的散射方向和能量比率，它和BRDF相同，都遵循亥姆霍兹互反律（Helmholtz’s law of reciprocity）
 * 相函数的单位是（1/sr），由于相函数定义在单位球体上，因此它的归一化与BRDF不同
 * \int_{\Omega 4\pi}p(x,\omega^{'}\to\omega)\mathrm{d}\omega
 */
class RAD_EXPORT_API PhaseFunction {
 public:
  virtual ~PhaseFunction() noexcept = default;

  /**
   * @brief 采样一个方向
   *
   * @param mi 介质
   * @param xi 采样
   * @return std::pair<Vector3, Float> [世界坐标系中的方向，pdf]
   */
  virtual std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) const = 0;
  /**
   * @brief 评估某个方向上的相函数
   * 
   * @param mi 介质
   * @param wo 世界坐标系中的方向
   * @return Float 相函数的值
   */
  virtual Float Eval(const MediumInteraction& mi, const Vector3& wo) const = 0;
};

}  // namespace Rad
