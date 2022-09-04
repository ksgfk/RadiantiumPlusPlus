#include <radiantium/math_ext.h>
#include <utility>

namespace rad::math {

Float Sqr(Float v) { return v * v; }
Float Sign(Float v) { return v >= 0 ? 1 : -1; }
Float MulSign(Float v1, Float v2) { return v2 >= 0 ? v1 : -v1; }

std::pair<Vec3, Vec3> CoordinateSystem(Vec3 n) {
  Float sign = Sign(n.z());
  Float a = -1 / (sign + n.z());
  Float b = n.x() * n.y() * a;
  Vec3 X = Vec3(
      MulSign(Sqr(n.x()) * a, n.z()) + 1.0f,
      MulSign(b, n.z()),
      MulSign(n.x(), -n.z()));
  Vec3 Y = Vec3(
      b,
      sign + Sqr(n.y()) * a,
      -n.y());
  return std::make_pair(X, Y);
}

}  // namespace rad::math