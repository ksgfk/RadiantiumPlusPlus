#include <rad/offline/frame.h>

#include <rad/offline/math_ext.h>

using namespace Rad::Math;

namespace Rad {

Frame::Frame(const Vector3& n) noexcept {
  std::tie(S, T) = CoordinateSystem(n);
  N = n;
}

Frame::Frame(const Vector3& s, const Vector3& t, const Vector3& n) noexcept
    : S(s), T(t), N(n) {}

Vector3 Frame::ToWorld(const Vector3& v) const {
  return N * v.z() + (T * v.y() + (S * v.x()));
}

Vector3 Frame::ToLocal(const Vector3& v) const {
  return Vector3(v.dot(S), v.dot(T), v.dot(N));
}

Float Frame::CosTheta(const Vector3& v) { return v.z(); }
Float Frame::AbsCosTheta(const Vector3& v) { return std::abs(v.z()); }
Float Frame::Cos2Theta(const Vector3& v) { return Sqr(v.z()); }
Float Frame::Sin2Theta(const Vector3& v) { return Fmadd(v.x(), v.x(), Sqr(v.y())); }
Float Frame::SinTheta(const Vector3& v) { return SafeSqrt(Sin2Theta(v)); }
Float Frame::TanTheta(const Vector3& v) { return SafeSqrt(Fmadd(-v.z(), v.z(), Float(1))) / v.z(); }
Float Frame::Tan2Theta(const Vector3& v) {
  return std::max(Fmadd(-v.z(), v.z(), 1), Float(0)) / Sqr(v.z());
}
Float Frame::CosPhi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? Float(1)
             : std::clamp(v.x() * invSinTheta, Float(-1), Float(1));
}
Float Frame::SinPhi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? Float(0)
             : std::clamp(v.y() * invSinTheta, Float(-1), Float(1));
}
std::pair<Float, Float> Frame::SinCosPhi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = Rsqrt(Sin2Theta(v));
  Vector2 result = v.head<2>() * invSinTheta;
  result = std::abs(sin2Theta) <= 4 * Epsilon<Float>()
               ? Vector2(1, 0)
               : result.cwiseMin(-1).cwiseMax(1);
  return {result.y(), result.x()};
}
Float Frame::Cos2Phi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? Float(1)
             : std::clamp(Sqr(v.x()) / sin2Theta, Float(-1), Float(1));
}
Float Frame::Sin2Phi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? Float(0)
             : std::clamp(Sqr(v.y()) / sin2Theta, Float(-1), Float(1));
}

bool Frame::IsSameHemisphere(const Vector3& x, const Vector3& y) {
  return CosTheta(x) * CosTheta(y) > 0;
}

}  // namespace Rad
