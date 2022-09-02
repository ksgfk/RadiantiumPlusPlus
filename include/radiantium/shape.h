#pragma once

#include <cstdint>

#include "radiantium.h"

namespace rad {

struct ShapeHitInfo {
  Vec2 UV;
  Float T;
  uint32_t PrimitiveId;
  uint32_t ShapeId;
};

class IShape {
 public:
  virtual ~IShape() noexcept = default;

  virtual size_t PrimitiveCount() = 0;
};

}  // namespace rad
