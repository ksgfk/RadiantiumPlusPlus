#pragma once

#include "radiantium.h"
#include <utility>

namespace rad::math {

constexpr Float PI = static_cast<Float>(3.1415926535897932);
constexpr Float Degree(Float value) { return value * (180.0f / PI); }
constexpr Float Radian(Float value) { return value * (PI / 180.0f); }
constexpr Float Float32Epsilon = 0x1p-24;
constexpr Float Float64Epsilon = 0x1p-53;
constexpr Float RayEpsilon = Float32Epsilon * 1500;

/**
 * @brief 平方
 */
Float Sqr(Float v);
/**
 * @brief 获取v的符号
 */
Float Sign(Float v);
/**
 * @brief 将v2的符号乘到v1上
 */
Float MulSign(Float v1, Float v2);
/**
 * @brief v的倒数
 */
Float Rcp(Float v);
/**
 * @brief a * b + c
 */
Float Fmadd(Float a, Float b, Float c);
/**
 * @brief a * b + c
 */
Vec3 Fmadd(Vec3 a, Vec3 b, Vec3 c);
/**
 * @brief 二次根的倒数
 */
Float Rsqrt(Float v);
/**
 * @brief 根据输入的N方向构建本地坐标系
 */
std::pair<Vec3, Vec3> CoordinateSystem(Vec3 n);
/**
 * @brief 左手视图矩阵
 */
Mat4 LookAtLeftHand(const Vec3& origin, const Vec3& target, const Vec3& up);
/**
 * @brief 计算传入值的sin与cos
 */
std::pair<Float, Float> SinCos(Float v);

}  // namespace rad::math
