#include <radiantium/math_ext.h>

#include <utility>
#include <cmath>

namespace rad::math {

Float Sqr(Float v) { return v * v; }
Float Sign(Float v) { return v >= 0 ? Float(1) : Float(-1); }
Float MulSign(Float v1, Float v2) { return v2 >= 0 ? v1 : -v1; }
Float Rcp(Float v) { return Float(1) / v; }
Float Fmadd(Float a, Float b, Float c) { return a * b + c; }
Vec3 Fmadd(Vec3 a, Vec3 b, Vec3 c) { return a.cwiseProduct(b) + c; }
Float Rsqrt(Float v) { return Rcp(std::sqrt(v)); }

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

std::pair<Float, Float> SinCos(Float v) {
  return std::make_pair(std::sin(v), std::cos(v));
}

std::tuple<bool, Float, Float> SolveQuadratic(Float a, Float b, Float c) {
  bool linearCase = a == 0.0;
  bool validLinear = linearCase && b != 0.0;
  Float x0 = -c / b, x1 = -c / b;
  Float discrim = b * b - Float(4) * a * c;
  bool validQuadratic = !linearCase && (discrim >= 0.0);
  if (validQuadratic) {
    Float rootDiscrim = std::sqrt(discrim);
    Float temp = Float(-0.5) * (b + std::copysign(rootDiscrim, b));
    Float x0p = temp / a;
    Float x1p = c / temp;
    Float x0m = std::min(x0p, x1p);
    Float x1m = std::max(x0p, x1p);
    x0 = linearCase ? x0 : x0m;
    x1 = linearCase ? x0 : x1m;
  }
  return std::make_tuple(validLinear || validQuadratic, x0, x1);
}

Float UnitAngleZ(const Vec3& v) {
  Float temp = asin(Float(0.5) * Vec3(v.x(), v.y(), v.z() - MulSign(Float(1), v.z())).norm()) * 2;
  return v.z() >= 0 ? temp : PI - temp;
}

}  // namespace rad::math
