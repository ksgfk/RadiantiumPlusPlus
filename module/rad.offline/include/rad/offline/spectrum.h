#pragma once

#include <rad/core/color.h>

#include "types.h"

namespace Rad {

/**
 * @brief RGB颜色
 */
struct RgbSpectrum : public Vector3 {
  using Scalar = Vector3::Scalar;
  static constexpr UInt32 ComponentCount = Vector3::SizeAtCompileTime;

  RgbSpectrum() noexcept : Vector3(0, 0, 0) {}
  RgbSpectrum(Float value) noexcept : Vector3(value, value, value) {}
  RgbSpectrum(Float r, Float g, Float b) noexcept : Vector3(r, g, b) {}
  RgbSpectrum(const Vector3& v) noexcept : Vector3(v) {}
  template <typename Derived>
  RgbSpectrum(const Eigen::ArrayBase<Derived>& p) noexcept : Vector3(p) {}
  template <typename Derived>
  RgbSpectrum& operator=(const Eigen::ArrayBase<Derived>& p) noexcept {
    this->Vector3::operator=(p);
    return *this;
  }

  inline bool IsBlack() const { return isZero(); }
  inline bool HasNaN() const { return hasNaN(); }
  inline bool HasInfinity() const { return std::isinf(coeff(0)) || std::isinf(coeff(0)) || std::isinf(coeff(0)); }
  inline bool HasNegative() const { return coeff(0) < 0 || coeff(0) < 0 || coeff(0) < 0; }
  /**
   * @brief 所有分量中的最大分量
   */
  inline Scalar MaxComponent() const { return maxCoeff(); }
  /**
   * @brief 转换为亮度
   */
  inline Scalar Luminance() const { return (R() * static_cast<Scalar>(0.212671)) +
                                           (G() * static_cast<Scalar>(0.715160)) +
                                           (B() * static_cast<Scalar>(0.072169)); }

  Scalar R() const { return this->operator[](0); }
  Scalar& R() { return this->operator[](0); }
  Scalar G() const { return this->operator[](1); }
  Scalar& G() { return this->operator[](1); }
  Scalar B() const { return this->operator[](2); }
  Scalar& B() { return this->operator[](2); }

  inline static MatrixX<Color24f> CastToRGB(const MatrixX<RgbSpectrum>& spec) {
    MatrixX<Color24f> result;
    result.resize(spec.rows(), spec.cols());
    for (int i = 0; i < result.size(); i++) {
      RgbSpectrum s = spec.coeff(i);
      result.coeffRef(i) = Color24f(Float32(s.R()), Float32(s.G()), Float32(s.B()));
    }
    return result;
  }
};

inline RgbSpectrum Clamp(const RgbSpectrum& v, const RgbSpectrum& l, const RgbSpectrum& r) {
  return RgbSpectrum(v.cwiseMax(l).cwiseMin(r));
}

inline RgbSpectrum Clamp(const RgbSpectrum& v, Float l, Float r) {
  return RgbSpectrum(v.cwiseMax(l).cwiseMin(r));
}

inline RgbSpectrum LerpSpectrum(const RgbSpectrum& a, const RgbSpectrum& b, Float t) {
  return RgbSpectrum(a + RgbSpectrum(t).cwiseProduct(b - a));
}

inline RgbSpectrum LerpSpectrum(const RgbSpectrum& a, const RgbSpectrum& b, const RgbSpectrum& t) {
  return RgbSpectrum(a + t.cwiseProduct(b - a));
}

inline RgbSpectrum ExpSpectrum(const RgbSpectrum& v) {
  return RgbSpectrum(std::exp(v.x()), std::exp(v.y()), std::exp(v.z()));
}

inline RgbSpectrum Color24fToSpectrum(const Color24f& color) {
  return RgbSpectrum(color.cast<Float>());
}

}  // namespace Rad

template <>
struct spdlog::fmt_lib::formatter<Rad::RgbSpectrum> {
  template <typename FormatContext>
  auto format(const Rad::RgbSpectrum& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
