#pragma once

#include "radiantium.h"

namespace rad {
/**
 * @brief R G B 光谱
 * 
 */
struct RgbSpectrum : Vec3 {
  RgbSpectrum() noexcept : Vec3(0, 0, 0) {}
  RgbSpectrum(Float value) noexcept : Vec3(value, value, value) {}
  RgbSpectrum(Float r, Float g, Float b) noexcept : Vec3(r, g, b) {}
  RgbSpectrum(const Vec3& v) noexcept : Vec3(v) {}
  template <typename Derived>
  RgbSpectrum(const Eigen::ArrayBase<Derived>& p) noexcept : Vec3(p) {}
  template <typename Derived>
  RgbSpectrum& operator=(const Eigen::ArrayBase<Derived>& p) noexcept {
    this->Vec3::operator=(p);
    return *this;
  }

  bool IsBlack() const;
  bool HasNaN() const;
  bool HasInfinity() const;
  bool HasNegative() const;
  /**
   * @brief 所有分量中的最大分量
   */
  Float MaxComponent() const;
  /**
   * @brief 转换为亮度
   */
  Float Luminance() const;

  Float R() const;
  Float& R();
  Float G() const;
  Float& G();
  Float B() const;
  Float& B();
};

using Spectrum = RgbSpectrum;

}  // namespace rad

template <>
struct spdlog::fmt_lib::formatter<rad::RgbSpectrum> {
  template <typename FormatContext>
  auto format(const rad::RgbSpectrum& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
