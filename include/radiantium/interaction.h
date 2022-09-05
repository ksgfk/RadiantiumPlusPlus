#pragma once

#include "radiantium.h"
#include "frame.h"
#include "ray.h"

#include <limits>

namespace rad {

class IShape;

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

  void ComputeShadingFrame();
  Vec3 ToWorld(const Vec3& v) const;
  Vec3 ToLocal(const Vec3& v) const;
};

struct HitShapeRecord {
  Float T = std::numeric_limits<Float>::infinity();
  Vec3 GeometryNormal;
  Vec2 PrimitiveUV;
  UInt32 PrimitiveIndex;
  UInt32 ShapeIndex;
  IShape* Shape = nullptr;

  SurfaceInteraction ComputeSurfaceInteraction(const Ray& ray) const;
};

}  // namespace rad
