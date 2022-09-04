#pragma once

#include <cstdint>
#include <stdint.h>
#include <memory>

#include "radiantium.h"
#include "model.h"
#include "transform.h"

namespace rad {

class IShape {
 public:
  virtual ~IShape() noexcept = default;

  virtual size_t PrimitiveCount() = 0;

  virtual void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) = 0;
};

std::unique_ptr<IShape> CreateMesh(const TriangleModel& model, const Transform& transform);

}  // namespace rad
