#pragma once

#include "radiantium.h"

namespace rad {

struct Transform {
  Transform();
  Transform(const Mat4& toWorld);

  /* Properties */
  bool IsValid() const;
  bool IsIdentity() const;
  bool HasScale() const;
  Vec3 TranslationToWorld() const;
  Vec3 TranslationToLocal() const;

  /* Methods */
  Vec3 ApplyAffineToWorld(const Vec3& v) const;
  Vec3 ApplyLinearToWorld(const Vec3& v) const;
  Vec3 ApplyNormalToWorld(const Vec3& v) const;

  Vec3 ApplyAffineToLocal(const Vec3& v) const;
  Vec3 ApplyLinearToLocal(const Vec3& v) const;
  
  /* Fields */
  Mat4 ToWorld;
  Mat4 ToLocal;
};

}  // namespace rad
