#pragma once

#include <cstdint>
#include <stdint.h>
#include <memory>

#include "radiantium.h"
#include "model.h"
#include "transform.h"
#include "interaction.h"
#include "object.h"
#include "fwd.h"

namespace rad {

class IShape : public Object {
 public:
  virtual ~IShape() noexcept = default;

  virtual size_t PrimitiveCount() = 0;

  virtual void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) = 0;
  virtual SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) = 0;
};

std::unique_ptr<IShape> CreateMesh(const TriangleModel& model, const Transform& transform);

namespace factory {
std::unique_ptr<IFactory> CreateMeshFactory();
}

}  // namespace rad
