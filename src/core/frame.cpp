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

}  // namespace rad
