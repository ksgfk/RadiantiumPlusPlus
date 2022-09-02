#include <radiantium/spectrum.h>

#include <cmath>

namespace rad {

bool RgbSpectrum::IsBlack() const {
  return isZero();
}

bool RgbSpectrum::HasNaN() const {
  return hasNaN();
}

bool RgbSpectrum::HasInfinity() const {
  return std::isinf(coeff(0)) || std::isinf(coeff(0)) || std::isinf(coeff(0));
}

bool RgbSpectrum::HasNegative() const {
  return coeff(0) < 0 || coeff(0) < 0 || coeff(0) < 0;
}

Float RgbSpectrum::MaxComponent() const {
  return maxCoeff();
}

Float RgbSpectrum::Luminance() const {
  return ColorToLuminance(*this);
}

Float RgbSpectrum::R() const { return this->operator[](0); }
Float& RgbSpectrum::R() { return this->operator[](0); }
Float RgbSpectrum::G() const { return this->operator[](1); }
Float& RgbSpectrum::G() { return this->operator[](1); }
Float RgbSpectrum::B() const { return this->operator[](2); }
Float& RgbSpectrum::B() { return this->operator[](2); }

Float ColorToLuminance(const RgbSpectrum& c) {
  return (c.R() * static_cast<Float>(0.212671)) + (c.G() * static_cast<Float>(0.715160)) + (c.B() * static_cast<Float>(0.072169));
}

}  // namespace rad
