#pragma once

#include "types.h"

namespace Rad {
/**
 * @brief 本地坐标系, N朝向(0, 0, 1)
 */
struct Frame {
  Frame() = default;
  Frame(const Vector3& n) noexcept;
  explicit Frame(const Vector3& s, const Vector3& t, const Vector3& n) noexcept;

  Vector3 ToWorld(const Vector3& v) const;
  Vector3 ToLocal(const Vector3& v) const;

  Vector3 S, T, N;

  static Float CosTheta(const Vector3& v);
  static Float AbsCosTheta(const Vector3& v);
  static Float Cos2Theta(const Vector3& v);
  static Float Sin2Theta(const Vector3& v);
  static Float SinTheta(const Vector3& v);
  static Float TanTheta(const Vector3& v);
  static Float Tan2Theta(const Vector3& v);
  static Float CosPhi(const Vector3& v);
  static Float SinPhi(const Vector3& v);
  static std::pair<Float, Float> SinCosPhi(const Vector3& v);
  static Float Cos2Phi(const Vector3& v);
  static Float Sin2Phi(const Vector3& v);
  static bool IsSameHemisphere(const Vector3& x, const Vector3& y);
};

}  // namespace Rad
