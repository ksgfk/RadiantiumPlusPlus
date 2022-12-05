#pragma once

#include <rad/core/config_node.h>

#include "interaction.h"
#include "phase_function.h"

#include <utility>

namespace Rad {

/**
 * @brief 参与介质
 * 参与介质（participating media）是大量极小的粒子分布在一个三维的空间里面，这些粒子会衰减和散射光线。
 * 体积散射（volume scattering）模型基于一个假设：光线在介质中散射是一个概率过程，而不是真的考虑光线与粒子碰撞
 * 在具有参与介质的环境种，影响radiance分布的主要过程有三个：
 *   吸收（Absorption）：由于光转化为另一种形式的能量，如热，辐射的减少
 *   发射（Emission）：从发光粒子出发，增加到环境中的radiance
 *   散射（Scattering）：由于与粒子的碰撞，在一个方向上的radiance被散射到其他方向，散射分为内散射和外散射
 */
class RAD_EXPORT_API Medium {
 public:
  Medium(BuildContext* ctx, const ConfigNode& cfg);
  virtual ~Medium() noexcept = default;

  /**
   * @brief 光线是否与介质包围盒相交
   */
  virtual std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const = 0;
  /**
   * @brief 用于delta tracking方法的主要变量
   */
  virtual Spectrum GetMajorant(const MediumInteraction& mi) const = 0;
  /**
   * @brief 散射系数
   *
   * @return std::tuple<Spectrum, Spectrum, Spectrum> [sigma_s, sigma_n, sigma_t]
   */
  virtual std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const = 0;

  /**
   * @brief 在介质中采样一个点
   *
   * @param ray 入射光线
   * @param sample 采样概率
   * @param channel 用于RGB渲染选择通道
   */
  MediumInteraction SampleInteraction(const Ray& ray, Float sample, UInt32 channel) const;
  /**
   * @brief 评估介质的透射率
   * 
   * @param mi 介质中的点
   * @param si 碰撞表面的点
   * @return std::pair<Spectrum, Spectrum> [透射率, 概率]
   */
  std::pair<Spectrum, Spectrum> EvalTrAndPdf(const MediumInteraction& mi, const SurfaceInteraction& si) const;

  const PhaseFunction& GetPhaseFunction() const noexcept { return *_phaseFunction.get(); }
  bool IsHomogeneous() const { return _isHomogeneous; }
  bool HasSpectralExtinction() const { return _hasSpectralExtinction; }

 protected:
  Unique<PhaseFunction> _phaseFunction;
  bool _isHomogeneous;
  bool _hasSpectralExtinction;
};

}  // namespace Rad
