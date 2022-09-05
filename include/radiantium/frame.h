#pragma once

#include "radiantium.h"

namespace rad {

struct Frame {
  Frame() = default;
  Frame(const Vec3& n) noexcept;

  Vec3 ToWorld(const Vec3& v) const;
  Vec3 ToLocal(const Vec3& v) const;

  Vec3 S, T, N;
};

}  // namespace rad