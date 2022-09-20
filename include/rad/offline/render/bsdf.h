#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

enum class TransportMode {
  Radiance,
  Importance,
};

enum class BsdfType : UInt32 {
  Empty = 0b000000,
  Diffuse = 0b000001,
  Glossy = 0b000010,
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
constexpr UInt32 operator+(BsdfType e) { return (UInt32)e; }
template <typename T>
constexpr auto HasFlag(T flags, BsdfType f) {
  return (flags & (UInt32)f) != 0u;
}

struct BsdfContext {
  TransportMode Mode = TransportMode::Radiance;
  UInt32 TypeMask = (UInt32)BsdfType::All;

  // constexpr bool IsEnable(BsdfType type) const { return HasFlag(TypeMask, type); }

  template <typename... T>  //可变参数模板 Variadic Template
  constexpr bool IsEnable(T... types) const {
    return (HasFlag(TypeMask, types) && ...);  //折叠表达式 Fold Expressions
  }
};

struct BsdfSampleResult {
  Vector3 Wo;
  Float Pdf = 0;
  Float Eta = 1;
  UInt32 TypeMask = (UInt32)BsdfType::Empty;

  constexpr bool HasType(BsdfType type) const {
    return HasFlag(TypeMask, type);
  }
};

class Bsdf {
 public:
  virtual ~Bsdf() noexcept = default;

  UInt32 Flags() const { return _flags; }

  virtual std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const = 0;

  virtual Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const = 0;

  virtual Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const = 0;

 protected:
  UInt32 _flags;
};

}  // namespace Rad
