#pragma once

#include "radiantium.h"

namespace rad {

struct Frame {
  Frame() = default;
  Frame(const Vec3& n) noexcept;

  Vec3 S, T, N;
};

}  // namespace rad