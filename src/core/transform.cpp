#include "radiantium/logger.h"
#include "radiantium/radiantium.h"
#include <radiantium/transform.h>

#include <stdexcept>

namespace rad {

Transform::Transform() {
  ToWorld = Mat4::Identity();
  ToLocal = Mat4::Identity();
}

Transform::Transform(const Mat4& toWorld) {
  ToWorld = toWorld;
  bool canInv;
  toWorld.computeInverseWithCheck(ToLocal, canInv);
  if (!canInv) {
    ToWorld = Mat4::Zero();
    ToLocal = Mat4::Zero();
    throw std::invalid_argument("mat hasn't inv mat");
  }
}

bool Transform::IsValid() const {
  return !ToWorld.isZero();
}

bool Transform::IsIdentity() const {
  return ToWorld.isIdentity();
}

bool Transform::HasScale() const {
  Float la2 = ApplyLinearToWorld(Vec3(1, 0, 0)).squaredNorm();
  Float lb2 = ApplyLinearToWorld(Vec3(0, 1, 0)).squaredNorm();
  Float lc2 = ApplyLinearToWorld(Vec3(0, 0, 1)).squaredNorm();
  return !Vec3(la2, lb2, lc2).isApproxToConstant(1, 0.0001f);
}

bool Transform::HasNonUniformScale() const {
  Float la2 = ApplyLinearToWorld(Vec3(1, 0, 0)).squaredNorm();
  Float lb2 = ApplyLinearToWorld(Vec3(0, 1, 0)).squaredNorm();
  Float lc2 = ApplyLinearToWorld(Vec3(0, 0, 1)).squaredNorm();
  return std::abs(la2 - lb2) > 0.0001f || std::abs(la2 - lc2) > 0.0001f;
}

Vec3 Transform::TranslationToWorld() const {
  return ApplyAffineToWorld(Vec3(0, 0, 0));
}

Vec3 Transform::TranslationToLocal() const {
  return ApplyAffineToLocal(Vec3(0, 0, 0));
}

Vec3 Transform::ApplyAffineToWorld(const Vec3& v) const {
  Vec4 result = ToWorld * Vec4(v[0], v[1], v[2], 1.0f);
  return result.head<3>() / result.w();
}

Vec3 Transform::ApplyLinearToWorld(const Vec3& v) const {
  return ToWorld.topLeftCorner<3, 3>() * v;
}

Vec3 Transform::ApplyNormalToWorld(const Vec3& v) const {
  return ToLocal.topLeftCorner<3, 3>().transpose() * v;
}

Vec3 Transform::ApplyAffineToLocal(const Vec3& v) const {
  Vec4 result = ToLocal * Vec4(v[0], v[1], v[2], 1.0f);
  return result.head<3>() / result.w();
}

Vec3 Transform::ApplyLinearToLocal(const Vec3& v) const {
  return ToLocal.topLeftCorner<3, 3>() * v;
}

}  // namespace rad
