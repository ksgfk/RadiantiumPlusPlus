#pragma once

#include "types.h"

#include <cmath>
#include <type_traits>

namespace Rad::Math {

constexpr Float32 PI_F = static_cast<Float32>(3.1415926535897932);
constexpr Float64 PI_D = static_cast<Float64>(3.1415926535897932);
template <typename T>
constexpr T GenericPI() {
  if constexpr (std::is_same_v<T, Float32>) {
    return PI_F;
  } else if constexpr (std::is_same_v<T, Float64>) {
    return PI_D;
  } else {
    static_assert(std::is_floating_point_v<T>, "T must be floating point");
    return T(PI_D);
  }
}
/**
 * @brief 弧度转角度
 */
template <typename T>
constexpr T Degree(T value) { return value * (T(180) / GenericPI<T>()); }
/**
 * @brief 角度转弧度
 */
template <typename T>
constexpr T Radian(T value) { return value * (GenericPI<T>() / T(180)); }
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
 * @brief 将v2的符号乘到v1上
 */
template <typename TA, typename TB>
constexpr TA MulSign(const TA& v1, const TB& v2) { return v2 >= 0 ? v1 : -v1; }
/**
 * @brief v的倒数
 */
template <typename T>
constexpr T Rcp(T v) { return T(1) / v; }
/**
 * @brief 判断v是不是2的n次方
 */
template <typename T>
constexpr bool IsPowOf2(T v) {
  return v && !(v & (v - 1));
}
/**
 * @brief a * b + c
 */
inline float Fmadd(float a, float b, float c) { return std::fma(a, b, c); }
/**
 * @brief a * b + c
 */
inline double Fmadd(double a, double b, double c) { return std::fma(a, b, c); }
/**
 * @brief 二次根的倒数
 */
template <typename T>
inline T Rsqrt(T v) { return Rcp(std::sqrt(v)); }
/**
 * @brief 计算传入值的sin与cos
 */
inline std::pair<float, float> SinCos(float v) {
  return std::make_pair(std::sin(v), std::cos(v));
}
/**
 * @brief 计算传入值的sin与cos
 */
inline std::pair<double, double> SinCos(double v) {
  return std::make_pair(std::sin(v), std::cos(v));
}
template <typename Value, size_t N>
constexpr Value RadHornerImpl___(const Value& x, const Value (&coeff)[N]) {
  Value accum = coeff[N - 1];
  for (size_t i = 1; i < N; i++) {
    accum = Fmadd(x, accum, coeff[N - 1 - i]);
  }
  return accum;
}
/**
 * @brief 多项式求值，Horner算法
 */
template <typename Value, typename... Number>
constexpr Value Horner(const Value& x, Number... n) {
  Value coeffs[]{Value(n)...};
  return RadHornerImpl___(x, coeffs);
}

}  // namespace Rad::Math
