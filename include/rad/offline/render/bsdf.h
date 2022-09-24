#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

/**
 * @brief 光线传输模式
 */
enum class TransportMode {
  /**
   * @brief 从光源出发的光线
   */
  Radiance,
  /**
   * @brief 从摄像机出发的光线
   */
  Importance,
};

/**
 * @brief BSDF的Lobe类型
 *
 */
enum class BsdfType : UInt32 {
  /**
   * @brief 默认值
   */
  Empty = 0b000000,
  Diffuse = 0b000001,
  Glossy = 0b000010,
  /**
   * @brief lobe的分布是delta分布
   * delta分布指的是: 如果使用数值积分, 任何评估点 x 具有非零值 y 的概率为零
   * 说人话就是, 我们希望这种函数在任何地方的值都是0, 除了完美反射方向
   * 所以除了采样这种函数, 其他方法都不能准确评估
   */
  Delta = 0b000100,
  Reflection = 0b001000,
  Transmission = 0b010000,

  All = Diffuse | Glossy | Delta | Reflection | Transmission
};
constexpr UInt32 operator|(BsdfType f1, BsdfType f2) {
  return (UInt32)f1 | (UInt32)f2;
}
constexpr UInt32 operator|(UInt32 f1, BsdfType f2) {
  return f1 | (UInt32)f2;
}
constexpr UInt32 operator&(BsdfType f1, BsdfType f2) {
  return (UInt32)f1 & (UInt32)f2;
}
constexpr UInt32 operator&(UInt32 f1, BsdfType f2) {
  return f1 & (UInt32)f2;
}
constexpr UInt32 operator~(BsdfType f1) { return ~(UInt32)f1; }
template <typename T>
constexpr auto HasFlag(T flags, BsdfType f) {
  return (flags & (UInt32)f) != 0u;
}

/**
 * @brief 用来描述评估BSDF时, 可以采样哪些lobe, 还有当前的光线传输模式
 */
struct BsdfContext {
  TransportMode Mode = TransportMode::Radiance;
  UInt32 TypeMask = (UInt32)BsdfType::All;

  template <typename... T>  //可变参数模板 Variadic Template
  constexpr bool IsEnable(T... types) const {
    return (HasFlag(TypeMask, types) && ...);  //折叠表达式 Fold Expressions
  }
};

struct BsdfSampleResult {
  /**
   * @brief 本地坐标系下的出射方向
   */
  Vector3 Wo;
  /**
   * @brief 这次采样的概率密度
   */
  Float Pdf = 0;
  /**
   * @brief 被采样方向介质的折射率
   */
  Float Eta = 1;
  /**
   * @brief 采样点的BsdfType
   */
  UInt32 TypeMask = (UInt32)BsdfType::Empty;

  constexpr bool HasType(BsdfType type) const {
    return HasFlag(TypeMask, type);
  }
};

/**
 * @brief Bidirectional Scattering Distribution Function
 */
class Bsdf {
 public:
  virtual ~Bsdf() noexcept = default;
  /**
   * @brief 包含的所有可能的BsdfType
   */
  UInt32 Flags() const { return _flags; }

  /**
   * @brief 根据表面信息与入射方向, 采样一个出射方向, 同时返回带 cosine 项函数评估结果
   */
  virtual std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const = 0;

  /**
   * @brief 评估BSDF函数 f(wi, wo) 或者它的伴随形式, 返回带 cosine 项的结果
   * delta分布的函数一定返回0
   */
  virtual Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const = 0;

  /**
   * @brief 计算 采样给定方向上, 单位立体角的概率密度
   * delta分布的函数一定返回0
   */
  virtual Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const = 0;

 protected:
  UInt32 _flags;
};

}  // namespace Rad
