#include <radiantium/frame.h>

#include <radiantium/math_ext.h>

namespace rad {

Frame::Frame(const Vec3& n) noexcept {
  std::tie(S, T) = math::CoordinateSystem(n);
  N = n;
}

Vec3 Frame::ToWorld(const Vec3& v) const {
  return N * v.z() + (T * v.y() + (S * v.x()));
}

Vec3 Frame::ToLocal(const Vec3& v) const {
  return Vec3(v.dot(S), v.dot(T), v.dot(N));
}

Float Frame::CosTheta(const Vec3& v) { return v.z(); }
Float Frame::AbsCosTheta(const Vec3& v) { return std::abs(v.z()); }
Float Frame::Cos2Theta(const Vec3& v) { return math::Sqr(v.z()); }
Float Frame::Sin2Theta(const Vec3& v) { return math::Fmadd(v.x(), v.x(), math::Sqr(v.y())); }
Float Frame::SinTheta(const Vec3& v) { return math::SafeSqrt(Sin2Theta(v)); }
Float Frame::TanTheta(const Vec3& v) { return math::SafeSqrt(math::Fmadd(-v.z(), v.z(), 1.0f)) / v.z(); }
Float Frame::Tan2Theta(const Vec3& v) {
  return std::max(math::Fmadd(-v.z(), v.z(), 1), Float(0)) / math::Sqr(v.z());
}
Float Frame::CosPhi(const Vec3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = math::Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 1.0f
             : std::clamp(v.x() * invSinTheta, -1.0f, 1.0f);
}
Float Frame::SinPhi(const Vec3& v) {
  Float sin2Theta = Sin2Theta(v);
  Float invSinTheta = math::Rsqrt(Sin2Theta(v));
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 0.0f
             : std::clamp(v.y() * invSinTheta, -1.0f, 1.0f);
}
Float Frame::Cos2Phi(const Vec3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 1.0f
             : std::clamp(math::Sqr(v.x()) / sin2Theta, -1.0f, 1.0f);
}
Float Frame::Sin2Phi(const Vec3& v) {
  Float sin2Theta = Sin2Theta(v);
  return std::abs(sin2Theta) <= 4 * std::numeric_limits<Float>::epsilon()
             ? 0.0f
             : std::clamp(math::Sqr(v.y()) / sin2Theta, -1.0f, 1.0f);
}

}  // namespace rad
