#pragma once

#include "types.h"

namespace Rad {
/**
 * @brief 矩阵变换
 *
 */
struct Transform {
  Transform();
  Transform(const Matrix4& toWorld);

  bool IsValid() const;
  bool IsIdentity() const;
  bool HasScale() const;
  bool HasNonUniformScale() const;
  Vector3 TranslationToWorld() const;
  Vector3 TranslationToLocal() const;

  /**
   * @brief 仿射变换到世界空间
   */
  Vector3 ApplyAffineToWorld(const Vector3& v) const;
  /**
   * @brief 线性变换到世界空间
   */
  Vector3 ApplyLinearToWorld(const Vector3& v) const;
  /**
   * @brief 变换法线到世界空间
   */
  Vector3 ApplyNormalToWorld(const Vector3& v) const;
  BoundingBox3 ApplyBoxToWorld(const BoundingBox3& box) const;
  /**
   * @brief 仿射变换到本地空间
   */
  Vector3 ApplyAffineToLocal(const Vector3& v) const;
  /**
   * @brief 线性变换到本地空间
   */
  Vector3 ApplyLinearToLocal(const Vector3& v) const;
  BoundingBox3 ApplyBoxToLocal(const BoundingBox3& box) const;

  /*字段*/
  Matrix4 ToWorld;
  Matrix4 ToLocal;
};

}  // namespace Rad
