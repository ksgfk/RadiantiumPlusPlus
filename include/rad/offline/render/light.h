#pragma once

#include "../common.h"
#include "interaction.h"
#include "sample_result.h"

namespace Rad {

enum class LightType : UInt32 {
  Empty = 0b000000,
  DeltaPosition = 0x000001,
  DeltaDirection = 0x000010,
  Infinite = 0x000100,
  Surface = 0x001000,

  Delta = DeltaPosition | DeltaDirection,
};
constexpr UInt32 operator|(LightType f1, LightType f2) {
  return (UInt32)f1 | (UInt32)f2;
}
constexpr UInt32 operator|(UInt32 f1, LightType f2) {
  return f1 | (UInt32)f2;
}
constexpr UInt32 operator&(LightType f1, LightType f2) {
  return (UInt32)f1 & (UInt32)f2;
}
constexpr UInt32 operator&(UInt32 f1, LightType f2) {
  return f1 & (UInt32)f2;
}
constexpr UInt32 operator~(LightType f1) { return ~(UInt32)f1; }
template <typename T>
constexpr auto HasFlag(T flags, LightType f) {
  return (flags & (UInt32)f) != 0u;
}

class Light {
 public:
  virtual ~Light() noexcept = default;

  UInt32 Flags() const { return _flag; }

  /**
   * @brief 判断光源是不是环境光
   */
  bool IsEnv() const {
    return HasFlag(_flag, LightType::Infinite) && !HasFlag(_flag, LightType::Delta);
  }
  /**
   * @brief 给面光源之类的, 需要在形状表面采样的光源用的, 生命周期不归Light管, 但保证Shape生命周期比Light长
   */
  void AttachShape(Shape* shape) { _shape = shape; }

  /**
   * @brief 给定一个光源上的点, 返回沿入射方向的反方向发出的 radiance
   */
  virtual Spectrum Eval(const SurfaceInteraction& si) const = 0;

  /**
   * @brief 给定世界空间中的一个参考点, 采样一个从参考点到光源的方向
   * 采样方向的概率密度定义在单位立体角上
   */
  virtual std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const = 0;
  /**
   * @brief 计算从参考点到光源上点的方向的概率密度, 如果光源是delta分布, 则一定返回0
   */
  virtual Float PdfDirection(
      const Interaction& ref,
      const DirectionSampleResult& dsr) const = 0;

  /**
   * @brief 采样一个光源上的点
   * 概率密度定义在单位面积上
   */
  virtual std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const = 0;
  /**
   * @brief 计算采样光源上的点的概率密度
   */
  virtual Float PdfPosition(const PositionSampleResult& psr) const = 0;

 protected:
  UInt32 _flag;
  Shape* _shape = nullptr;
};

}  // namespace Rad