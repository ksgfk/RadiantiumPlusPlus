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

}  // namespace Rad
