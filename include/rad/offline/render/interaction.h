#pragma once

#include "../common.h"
#include "sample_result.h"

#include <limits>

namespace Rad {

struct Interaction {
  /**
   * @brief 交点距离
   */
  Float T = std::numeric_limits<Float>::infinity();
  /**
   * @brief 交点坐标
   */
  Vector3 P;
  /**
   * @brief 交点的法线方向
   */
  Vector3 N;

  Ray SpawnRay(const Vector3& d) const;
  Ray SpawnRayTo(const Vector3& p) const;
  bool IsValid() const;

 private:
  Vector3 OffsetP(const Vector3& d) const;
};

struct SurfaceInteraction : public Interaction {
  /**
   * @brief 相交的形状
   */
  Shape* Shape = nullptr;
  UInt32 ShapeIndex = std::numeric_limits<UInt32>::max();
  /**
   * @brief 着色法线所在的本地坐标系
   */
  Frame Shading;
  Vector2 UV;
  Vector3 dPdU, dPdV;
  Vector3 dNdU, dNdV;
  /**
   * @brief 本地坐标系下的入射方向
   */
  Vector3 Wi;

  SurfaceInteraction() = default;
  SurfaceInteraction(const PositionSampleResult& dsr);

  void ComputeShadingFrame();
  Vector3 ToWorld(const Vector3& v) const;
  Vector3 ToLocal(const Vector3& v) const;

  PositionSampleResult ToPsr() const;
  DirectionSampleResult ToDsr(const Interaction& ref) const;
};

struct HitShapeRecord {
  Float T = std::numeric_limits<Float>::infinity();
  Vector3 GeometryNormal;
  Vector2 PrimitiveUV;
  UInt32 PrimitiveIndex;
  Shape* ShapePtr = nullptr;
  UInt32 ShapeIndex = std::numeric_limits<UInt32>::max();

  SurfaceInteraction ComputeSurfaceInteraction(const Ray& ray) const;
};

}  // namespace Rad