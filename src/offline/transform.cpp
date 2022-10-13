#include <rad/offline/transform.h>

namespace Rad {

Transform::Transform() {
  ToWorld = Matrix4::Identity();
  ToLocal = Matrix4::Identity();
}

Transform::Transform(const Matrix4& toWorld) {
  ToWorld = toWorld;
  bool canInv;
  toWorld.computeInverseWithCheck(ToLocal, canInv, 0);
  if (!canInv) {
    ToWorld = Matrix4::Zero();
    ToLocal = Matrix4::Zero();
    throw RadArgumentException("矩阵不可逆 {}", toWorld);
  }
}

bool Transform::IsValid() const {
  return !ToWorld.isZero();
}

bool Transform::IsIdentity() const {
  return ToWorld.isIdentity();
}

bool Transform::HasScale() const {
  Float la2 = ApplyLinearToWorld(Vector3(1, 0, 0)).squaredNorm();
  Float lb2 = ApplyLinearToWorld(Vector3(0, 1, 0)).squaredNorm();
  Float lc2 = ApplyLinearToWorld(Vector3(0, 0, 1)).squaredNorm();
  return !Vector3(la2, lb2, lc2).isApproxToConstant(1, 0.0001f);
}

bool Transform::HasNonUniformScale() const {
  Float la2 = ApplyLinearToWorld(Vector3(1, 0, 0)).squaredNorm();
  Float lb2 = ApplyLinearToWorld(Vector3(0, 1, 0)).squaredNorm();
  Float lc2 = ApplyLinearToWorld(Vector3(0, 0, 1)).squaredNorm();
  return std::abs(la2 - lb2) > 0.0001f || std::abs(la2 - lc2) > 0.0001f;
}

Vector3 Transform::TranslationToWorld() const {
  return Eigen::Transform<Float, 3, Eigen::Affine>(ToWorld).translation();
}

Vector3 Transform::TranslationToLocal() const {
  return Eigen::Transform<Float, 3, Eigen::Affine>(ToLocal).translation();
}

Vector3 Transform::ApplyAffineToWorld(const Vector3& v) const {
  Vector4 result = ToWorld * Vector4(v[0], v[1], v[2], 1.0f);
  return result.head<3>() / result.w();
}

Vector3 Transform::ApplyLinearToWorld(const Vector3& v) const {
  return Eigen::Transform<Float, 3, Eigen::Affine>(ToWorld).linear() * v;
}

Vector3 Transform::ApplyNormalToWorld(const Vector3& v) const {
  return Eigen::Transform<Float, 3, Eigen::Affine>(ToLocal).linear().transpose() * v;
}

Vector3 Transform::ApplyAffineToLocal(const Vector3& v) const {
  Vector4 result = ToLocal * Vector4(v[0], v[1], v[2], 1.0f);
  return result.head<3>() / result.w();
}

Vector3 Transform::ApplyLinearToLocal(const Vector3& v) const {
  return Eigen::Transform<Float, 3, Eigen::Affine>(ToLocal).linear() * v;
}

}  // namespace Rad
