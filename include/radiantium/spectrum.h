#pragma once

#include "radiantium.h"

namespace rad {

struct RgbSpectrum : Vec3 {
  RgbSpectrum() noexcept : Vec3(0, 0, 0) {}
  RgbSpectrum(float value) noexcept : Vec3(value, value, value) {}
  RgbSpectrum(float r, float g, float b) noexcept : Vec3(r, g, b) {}
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
  Float MaxComponent() const;
  Float Luminance() const;

  Float R() const;
  Float& R();
  Float G() const;
  Float& G();
  Float B() const;
  Float& B();

  // template <typename OStream>
  // friend OStream& operator<<(OStream& os, const RgbSpectrum& c) {
  //   return os << "<" << c.R() << ", " << c.G() << ", " << c.B() << ">";
  // }
};

Float ColorToLuminance(const RgbSpectrum& c);

using Spectrum = RgbSpectrum;

}  // namespace rad
