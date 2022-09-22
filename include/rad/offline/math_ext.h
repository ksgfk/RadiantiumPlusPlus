#pragma once

#include "types.h"

#include <type_traits>

namespace Rad::Math {

constexpr Float PI = static_cast<Float>(3.1415926535897932);
constexpr Float Degree(Float value) { return value * (180.0f / PI); }
constexpr Float Radian(Float value) { return value * (PI / 180.0f); }
constexpr Float32 Float32Epsilon = 0x1p-24;
constexpr Float64 Float64Epsilon = 0x1p-53;
template <typename T>
constexpr T Epsilon() {
  if constexpr (std::is_same_v<T, Float32>) {
    return Float32Epsilon;
  } else if constexpr (std::is_same_v<T, Float64>) {
    return Float64Epsilon;
  } else {
    return T(Float32Epsilon);
  }
}
constexpr Float RayEpsilon = Epsilon<Float>() * Float(1500);
constexpr Float ShadowEpsilon = RayEpsilon * Float(10);
template <typename T>
constexpr T OneMinusEpsilon() {
  if constexpr (std::is_same_v<T, Float32>) {
    return T(0x1.fffffep-1);
  } else if constexpr (std::is_same_v<T, Float64>) {
    return T(0x1.fffffffffffffp-1);
  } else {
    return T(0x1.fffffep-1);
  }
}

/**
 * @brief 平方
 */
template <typename T>
constexpr T Sqr(T v) { return v * v; }
/**
 * @brief 获取v的符号
 */
template <typename T>
constexpr T Sign(T v) { return v >= 0 ? T(1) : T(-1); }
/**
 * @brief 将v2的符号乘到v1上
 */
template <typename T>
constexpr T MulSign(T v1, T v2) { return v2 >= 0 ? v1 : -v1; }
/**
 * @brief v的倒数
 */
template <typename T>
constexpr T Rcp(T v) { return T(1) / v; }
/**
 * @brief a * b + c
 */
constexpr Float Fmadd(Float a, Float b, Float c) { return a * b + c; }
/**
 * @brief a * b + c
 */
inline Vector3 Fmadd(const Vector3& a, const Vector3& b, const Vector3& c) {
  return a.cwiseProduct(b) + c;
}
/**
 * @brief 二次根的倒数
 */
template <typename T>
inline T Rsqrt(T v) { return Rcp(std::sqrt(v)); }
/**
 * @brief 安全的计算平方根, 如果输入负数返回0
 */
inline Float SafeSqrt(Float v) { return std::sqrt(std::max(v, Float(0))); }
inline Vector3 SafeSqrt(const Vector3& v) {
  return v.cwiseMax(0).cwiseSqrt();
}
/**
 * @brief 绝对值点乘
 */
inline Float AbsDot(const Vector3& v1, const Vector3& v2) {
  return std::abs(v1.dot(v2));
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
 * @brief 计算传入值的sin与cos
 */
inline std::pair<Float, Float> SinCos(Float v) {
  return std::make_pair(std::sin(v), std::cos(v));
}
/**
 * @brief 解二元一次方程
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

}  // namespace Rad::Math
