#pragma once

#include "types.h"

namespace Rad {

/**
 * @brief RGB颜色
 */
struct Color : Eigen::Vector3f {
  using Scalar = Eigen::Vector3f::Scalar;

  Color() noexcept : Eigen::Vector3f(0, 0, 0) {}
  Color(Float32 value) noexcept : Eigen::Vector3f(value, value, value) {}
  Color(Float32 r, Float32 g, Float32 b) noexcept : Eigen::Vector3f(r, g, b) {}
  Color(const Eigen::Vector3f& v) noexcept : Eigen::Vector3f(v) {}
  template <typename Derived>
  Color(const Eigen::ArrayBase<Derived>& p) noexcept : Eigen::Vector3f(p) {}
  template <typename Derived>
  Color& operator=(const Eigen::ArrayBase<Derived>& p) noexcept {
    this->Eigen::Vector3f::operator=(p);
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

  static Color ToLinear(const Color& color);
  static Color ToSRGB(const Color& color);
};

inline Color Clamp(const Color& v, const Color& l, const Color& r) {
  return Color(v.cwiseMax(l).cwiseMin(r));
}

inline Color Clamp(const Color& v, Float32 l, Float32 r) {
  return Color(v.cwiseMax(l).cwiseMin(r));
}

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
