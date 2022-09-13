#pragma once

#include "radiantium.h"

namespace rad {
/**
 * @brief 本地坐标系, N朝向(0, 0, 1)
 */
struct Frame {
  Frame() = default;
  Frame(const Vec3& n) noexcept;

  Vec3 ToWorld(const Vec3& v) const;
  Vec3 ToLocal(const Vec3& v) const;

  Vec3 S, T, N;

  static Float CosTheta(const Vec3& v);
  static Float AbsCosTheta(const Vec3& v);
  static Float Cos2Theta(const Vec3& v);
  static Float Sin2Theta(const Vec3& v);
  static Float SinTheta(const Vec3& v);
  static Float TanTheta(const Vec3& v);
  static Float Tan2Theta(const Vec3& v);
  static Float CosPhi(const Vec3& v);
  static Float SinPhi(const Vec3& v);
  static Float Cos2Phi(const Vec3& v);
  static Float Sin2Phi(const Vec3& v);
};

}  // namespace rad
