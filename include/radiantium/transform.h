#pragma once

#include "radiantium.h"

namespace rad {
/**
 * @brief 矩阵变换
 *
 */
struct Transform {
  Transform();
  Transform(const Mat4& toWorld);

  /*属性*/
  bool IsValid() const;
  bool IsIdentity() const;
  bool HasScale() const;
  bool HasNonUniformScale() const;
  Vec3 TranslationToWorld() const;
  Vec3 TranslationToLocal() const;

  /*方法*/
  /**
   * @brief 仿射变换到世界空间
   */
  Vec3 ApplyAffineToWorld(const Vec3& v) const;
  /**
   * @brief 线性变换到世界空间
   */
  Vec3 ApplyLinearToWorld(const Vec3& v) const;
  /**
   * @brief 变换法线到世界空间
   */
  Vec3 ApplyNormalToWorld(const Vec3& v) const;
  /**
   * @brief 仿射变换到本地空间
   */
  Vec3 ApplyAffineToLocal(const Vec3& v) const;
  /**
   * @brief 线性变换到本地空间
   */
  Vec3 ApplyLinearToLocal(const Vec3& v) const;

  /*字段*/
  Mat4 ToWorld;
  Mat4 ToLocal;
};

}  // namespace rad
