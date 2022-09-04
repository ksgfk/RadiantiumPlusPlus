#pragma once

#include "radiantium.h"
#include "radiantium/frame.h"
#include <limits>

namespace rad {

class IShape;

struct HitShapeRecord {
  Float T = std::numeric_limits<Float>::infinity();
  Vec2 PrimitiveUV;
  UInt32 PrimitiveIndex;
  UInt32 ShapeIndex;
  IShape* Shape = nullptr;
};

struct Interaction {
  Float T;
  Vec3 P;
  Vec3 N;
};

struct SurfaceInteraction : public Interaction {
  IShape* Shape;
  Frame Shading;
  Vec2 UV;
  Vec3 dPdU, dPdV;
  Vec3 dNdU, dNdV;
  Vec3 Wi;
  UInt32 PrimitiveIndex;
};

}  // namespace rad
