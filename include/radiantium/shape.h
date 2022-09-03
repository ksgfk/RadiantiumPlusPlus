#pragma once

#include <cstdint>
#include <stdint.h>

#include "radiantium.h"

namespace rad {

struct ShapeHitInfo {
  Vec2 UV;
  Float T;
  UInt32 PrimitiveId;
  UInt32 ShapeId;
};

class IShape {
 public:
  virtual ~IShape() noexcept = default;

  UInt32 GetId() const { return _id; }
  void SetId(UInt32 id) { _id = id; }

  virtual size_t PrimitiveCount() = 0;

 protected:
  UInt32 _id;
};

}  // namespace rad
