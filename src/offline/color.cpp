#include <rad/offline/color.h>

#include <cmath>

namespace Rad {

using Scalar = Color::Scalar;

bool Color::IsBlack() const {
  return isZero();
}

bool Color::HasNaN() const {
  return hasNaN();
}

bool Color::HasInfinity() const {
  return std::isinf(coeff(0)) || std::isinf(coeff(0)) || std::isinf(coeff(0));
}

bool Color::HasNegative() const {
  return coeff(0) < 0 || coeff(0) < 0 || coeff(0) < 0;
}

Scalar Color::MaxComponent() const {
  return maxCoeff();
}

Scalar Color::Luminance() const {
  return (R() * static_cast<Scalar>(0.212671)) +
         (G() * static_cast<Scalar>(0.715160)) +
         (B() * static_cast<Scalar>(0.072169));
}

Scalar Color::R() const { return this->operator[](0); }
Scalar& Color::R() { return this->operator[](0); }
Scalar Color::G() const { return this->operator[](1); }
Scalar& Color::G() { return this->operator[](1); }
Scalar Color::B() const { return this->operator[](2); }
Scalar& Color::B() { return this->operator[](2); }

Float ToLinearImpl(Float value) {
  return value <= 0.04045f ? value * (1.0f / 12.92f) : std::pow((value + 0.055f) * (1.0f / 1.055f), 2.4f);
}
Color Color::ToLinear(const Color& color) {
  return Color(ToLinearImpl(color.x()), ToLinearImpl(color.y()), ToLinearImpl(color.z()));
}

Float ToSrgb(Float value) {
  return value <= 0.0031308f ? 12.92f * value : (1.0f + 0.055f) * std::pow(value, 1.0f / 2.4f) - 0.055f;
}
Color Color::ToSRGB(const Color& color) {
  return Color(ToSrgb(color.x()), ToSrgb(color.y()), ToSrgb(color.z()));
}

}  // namespace Rad
