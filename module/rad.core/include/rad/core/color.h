#pragma once

#include "types.h"
#include "math_base.h"

#include <cmath>

namespace Rad {

/**
 * @brief 以float储存的RGB颜色
 */
class Color24f : public Eigen::Vector3f {
 public:
  using Base = Eigen::Vector3f;

  Color24f(void) : Base() {}
  Color24f(Float32 value) noexcept : Base(value, value, value) {}
  Color24f(Float32 r, Float32 g, Float32 b) noexcept : Base(r, g, b) {}
  template <typename Derived>
  Color24f(const Eigen::MatrixBase<Derived>& other) : Base(other) {}
  template <typename Derived>
  Color24f& operator=(const Eigen::MatrixBase<Derived>& other) {
    this->Base::operator=(other);
    return *this;
  }

  inline bool IsBlack() const { return isZero(); }
  inline bool HasNaN() const { return hasNaN(); }
  inline bool HasInfinity() const { return std::isinf(coeff(0)) || std::isinf(coeff(0)) || std::isinf(coeff(0)); }
  inline bool HasNegative() const { return coeff(0) < 0 || coeff(0) < 0 || coeff(0) < 0; }
  inline Scalar MaxComponent() const { return maxCoeff(); }
  inline Scalar Luminance() const { return (R() * static_cast<Scalar>(0.212671)) + (G() * static_cast<Scalar>(0.715160)) + (B() * static_cast<Scalar>(0.072169)); }
  inline Scalar R() const { return x(); }
  inline Scalar& R() { return x(); }
  inline Scalar G() const { return y(); }
  inline Scalar& G() { return y(); }
  inline Scalar B() const { return z(); }
  inline Scalar& B() { return z(); }

  inline static Color24f ToLinear(const Color24f& color) {
    auto impl = [](Float32 value) {
      return value <= 0.04045f ? value * (1.0f / 12.92f) : std::pow((value + 0.055f) * (1.0f / 1.055f), 2.4f);
    };
    return Color24f(impl(color.x()), impl(color.y()), impl(color.z()));
  }
  inline static Color24f ToSRGB(const Color24f& color) {
    auto impl = [](Float32 value) {
      return value <= 0.0031308f ? 12.92f * value : (1.0f + 0.055f) * std::pow(value, 1.0f / 2.4f) - 0.055f;
    };
    return Color24f(impl(color.x()), impl(color.y()), impl(color.z()));
  }
};
inline auto Clamp(const Color24f& v, const Color24f& l, const Color24f& r) {
  return v.cwiseMax(l).cwiseMin(r);
}
inline auto Clamp(const Color24f& v, Float32 l, Float32 r) {
  return v.cwiseMax(l).cwiseMin(r);
}

}  // namespace Rad

template <>
struct spdlog::fmt_lib::formatter<Rad::Color24f> {
  template <typename FormatContext>
  auto format(const Rad::Color24f& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
