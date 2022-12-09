#pragma once

#include <rad/core/math_base.h>
#include "types.h"

namespace Rad::Math {

constexpr Float PI = static_cast<Float>(3.1415926535897932);
constexpr Float32 Float32Epsilon = 0x1p-24;
template <typename T>
constexpr T Epsilon() { return T(Float32Epsilon); }
constexpr Float RayEpsilon = Epsilon<Float>() * Float(1500);
/**
 * @brief 光线求交时会有浮点误差，为了避免出现自相交的情况，需要将点偏移一点点距离
 */
constexpr Float ShadowEpsilon = RayEpsilon * Float(10);
template <typename T>
constexpr T OneMinusEpsilon() { return T(0x1.fffffep-1); }
/**
 * @brief a * b + c
 */
inline Vector3 Fmadd(const Vector3& a, const Vector3& b, const Vector3& c) {
  return Vector3(
      Fmadd(a.x(), b.x(), c.x()),
      Fmadd(a.y(), b.y(), c.y()),
      Fmadd(a.z(), b.z(), c.z()));
}
/**
 * @brief 安全的计算平方根, 如果输入负数返回0
 */
inline Float SafeSqrt(Float v) { return std::sqrt(std::max(v, Float(0))); }
template <typename Derived>
inline auto SafeSqrt(const Eigen::ArrayBase<Derived>& v) {
  return v.cwiseMax(0).cwiseSqrt();
}
/**
 * @brief 绝对值点乘
 */
inline Float AbsDot(const Vector3& v1, const Vector3& v2) {
  return std::abs(v1.dot(v2));
}
inline Float Lerp(Float a, Float b, Float t) {
  return Fmadd(b, t, Fmadd(-a, t, a));
}
/**
 * @brief 根据输入的N方向构建本地坐标系
 */
inline std::pair<Vector3, Vector3> CoordinateSystem(const Vector3& n) {
  Float sign = Sign(n.z());
  Float a = -1 / (sign + n.z());
  Float b = n.x() * n.y() * a;
  Vector3 X = Vector3(
      MulSign(Sqr(n.x()) * a, n.z()) + 1.0f,
      MulSign(b, n.z()),
      MulSign(n.x(), -n.z()));
  Vector3 Y = Vector3(
      b,
      sign + Sqr(n.y()) * a,
      -n.y());
  return std::make_pair(X, Y);
}
/**
 * @brief 左手视图矩阵
 */
inline Matrix4 LookAtLeftHand(const Vector3& origin, const Vector3& target, const Vector3& up) {
  Vector3 dir = (target - origin).normalized();
  Vector3 left = up.normalized().cross(dir).normalized();
  Vector3 realUp = dir.cross(left).normalized();
  Matrix4 result;
  result << left.x(), realUp.x(), dir.x(), origin.x(),
      left.y(), realUp.y(), dir.y(), origin.y(),
      left.z(), realUp.z(), dir.z(), origin.z(),
      0, 0, 0, 1;
  return result;
}
/**
 * @brief 解一元二次方程
 */
template <typename Float>
inline std::tuple<bool, Float, Float> SolveQuadratic(Float a, Float b, Float c) {
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
/**
 * @brief 计算acos(v.z()), 实际上就是计算向量v与z轴夹角
 */
inline Float UnitAngleZ(const Vector3& v) {
  Float temp = asin(Float(0.5) * Vector3(v.x(), v.y(), v.z() - MulSign(Float(1), v.z())).norm()) * 2;
  return v.z() >= 0 ? temp : PI - temp;
}
/**
 * @brief 从球面坐标转化为直角坐标
 */
inline Vector3 SphericalCoordinates(Float theta, Float phi) {
  auto [sinTheta, cosTheta] = SinCos(theta);
  auto [sinPhi, cosPhi] = SinCos(phi);
  return Vector3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
}
/**
 * @brief 近似erf函数的逆函数
 */
inline Float ErfInv(Float a) {
  float p;
  float t = (float)std::log(std::max(Fmadd(a, -a, 1), std::numeric_limits<Float>::min()));
  if (std::abs(t) > 6.125f) {          // maximum ulp error = 2.35793
    p = 3.03697567e-10f;               //  0x1.4deb44p-32
    p = Fmadd(p, t, 2.93243101e-8f);   //  0x1.f7c9aep-26
    p = Fmadd(p, t, 1.22150334e-6f);   //  0x1.47e512p-20
    p = Fmadd(p, t, 2.84108955e-5f);   //  0x1.dca7dep-16
    p = Fmadd(p, t, 3.93552968e-4f);   //  0x1.9cab92p-12
    p = Fmadd(p, t, 3.02698812e-3f);   //  0x1.8cc0dep-9
    p = Fmadd(p, t, 4.83185798e-3f);   //  0x1.3ca920p-8
    p = Fmadd(p, t, -2.64646143e-1f);  // -0x1.0eff66p-2
    p = Fmadd(p, t, 8.40016484e-1f);   //  0x1.ae16a4p-1
  } else {                             // maximum ulp error = 2.35456
    p = 5.43877832e-9f;                //  0x1.75c000p-28
    p = Fmadd(p, t, 1.43286059e-7f);   //  0x1.33b458p-23
    p = Fmadd(p, t, 1.22775396e-6f);   //  0x1.49929cp-20
    p = Fmadd(p, t, 1.12962631e-7f);   //  0x1.e52bbap-24
    p = Fmadd(p, t, -5.61531961e-5f);  // -0x1.d70c12p-15
    p = Fmadd(p, t, -1.47697705e-4f);  // -0x1.35be9ap-13
    p = Fmadd(p, t, 2.31468701e-3f);   //  0x1.2f6402p-9
    p = Fmadd(p, t, 1.15392562e-2f);   //  0x1.7a1e4cp-7
    p = Fmadd(p, t, -2.32015476e-1f);  // -0x1.db2aeep-3
    p = Fmadd(p, t, 8.86226892e-1f);   //  0x1.c5bf88p-1
  }
  return a * p;
}

}  // namespace Rad::Math
