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
constexpr UInt32 operator+(LightType e) { return (UInt32)e; }
template <typename T>
constexpr auto HasFlag(T flags, LightType f) {
  return (flags & (UInt32)f) != 0u;
}

class Light {
 public:
  virtual ~Light() noexcept = default;

  UInt32 Flags() const { return _flag; }

  bool IsEnv() const {
    return HasFlag(_flag, LightType::Infinite) && !HasFlag(_flag, LightType::Delta);
  }

  void AttachShape(Shape* shape) { _shape = shape; }

  virtual Spectrum Eval(const SurfaceInteraction& si) const = 0;

  virtual std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const = 0;
  virtual Float PdfDirection(
      const Interaction& ref,
      const DirectionSampleResult& dsr) const = 0;
  virtual Spectrum EvalDirection(
      const Interaction& ref,
      const DirectionSampleResult& dsr) const = 0;

  virtual std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const = 0;
  virtual Float PdfPosition(const PositionSampleResult& psr) const = 0;

 protected:
  UInt32 _flag;
  Shape* _shape = nullptr;
};

}  // namespace Rad
