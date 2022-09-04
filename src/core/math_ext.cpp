#include <radiantium/math_ext.h>
#include <utility>

namespace rad::math {

Float Sqr(Float v) { return v * v; }
Float Sign(Float v) { return v >= 0 ? Float(1) : Float(-1); }
Float MulSign(Float v1, Float v2) { return v2 >= 0 ? v1 : -v1; }
Float Rcp(Float v) { return Float(1) / v; }

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

Mat4 LookAtLeftHand(const Vec3& origin, const Vec3& target, const Vec3& up) {
  Vec3 dir = (target - origin).normalized();
  Vec3 left = up.normalized().cross(dir).normalized();
  Vec3 realUp = dir.cross(left).normalized();
  Mat4 result;
  result << left.x(), realUp.x(), dir.x(), origin.x(),
      left.y(), realUp.y(), dir.y(), origin.y(),
      left.z(), realUp.z(), dir.z(), origin.z(),
      0, 0, 0, 1;
  return result;
}

}  // namespace rad::math
