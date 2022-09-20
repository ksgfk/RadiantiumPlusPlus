#include <rad/offline/frame.h>

#include <rad/offline/math_ext.h>

using namespace Rad::Math;

namespace Rad {

Frame::Frame(const Vector3& n) noexcept {
  std::tie(S, T) = CoordinateSystem(n);
  N = n;
}

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
Float Frame::TanTheta(const Vector3& v) { return SafeSqrt(Fmadd(-v.z(), v.z(), 1.0f)) / v.z(); }
Float Frame::Tan2Theta(const Vector3& v) {
  return std::max(Fmadd(-v.z(), v.z(), 1), Float(0)) / Sqr(v.z());
}
Float Frame::CosPhi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 1.0f
             : std::clamp(v.x() * invSinTheta, -1.0f, 1.0f);
}
Float Frame::SinPhi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 0.0f
             : std::clamp(v.y() * invSinTheta, -1.0f, 1.0f);
}
Float Frame::Cos2Phi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 1.0f
             : std::clamp(Sqr(v.x()) / sin2Theta, -1.0f, 1.0f);
}
Float Frame::Sin2Phi(const Vector3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 0.0f
             : std::clamp(Sqr(v.y()) / sin2Theta, -1.0f, 1.0f);
}

bool Frame::IsSameHemisphere(const Vector3& x, const Vector3& y) {
  return CosTheta(x) * CosTheta(y) > 0;
}

}  // namespace Rad
