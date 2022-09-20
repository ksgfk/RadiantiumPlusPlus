#pragma once

#include "types.h"

namespace Rad {

struct Color : Vector3 {
  using Scalar = Vector3::Scalar;

  Color() noexcept : Vector3(0, 0, 0) {}
  Color(Float value) noexcept : Vector3(value, value, value) {}
  Color(Float r, Float g, Float b) noexcept : Vector3(r, g, b) {}
  Color(const Vector3& v) noexcept : Vector3(v) {}
  template <typename Derived>
  Color(const Eigen::ArrayBase<Derived>& p) noexcept : Vector3(p) {}
  template <typename Derived>
  Color& operator=(const Eigen::ArrayBase<Derived>& p) noexcept {
    this->Vector3::operator=(p);
    return *this;
  }

  bool IsBlack() const;
  bool HasNaN() const;
  bool HasInfinity() const;
  bool HasNegative() const;
  /**
   * @brief 所有分量中的最大分量
   */
  Scalar MaxComponent() const;
  /**
   * @brief 转换为亮度
   */
  Scalar Luminance() const;

  Scalar R() const;
  Scalar& R();
  Scalar G() const;
  Scalar& G();
  Scalar B() const;
  Scalar& B();
};

inline Color Clamp(const Color& v, const Color& l, const Color& r) {
  return Color(v.cwiseMax(l).cwiseMin(r));
}

inline Color Clamp(const Color& v, Float l, Float r) {
  return Color(v.cwiseMax(l).cwiseMin(r));
}

using Spectrum = Color;

}  // namespace Rad

template <>
struct spdlog::fmt_lib::formatter<Rad::Color> {
  template <typename FormatContext>
  auto format(const Rad::Color& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
