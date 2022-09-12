#pragma once

#include "radiantium.h"
#include "frame.h"
#include "ray.h"

#include <limits>

namespace rad {

class IShape;

struct Interaction {
  /**
   * @brief 交点距离
   */
  Float T;
  /**
   * @brief 交点坐标
   *
   */
  Vec3 P;
  /**
   * @brief 交点的法线方向
   */
  Vec3 N;

  Ray SpawnRay(const Vec3& d) const;

 private:
  Vec3 OffsetP(const Vec3& d) const;
};

struct SurfaceInteraction : public Interaction {
  /**
   * @brief 相交的形状
   */
  IShape* Shape;
  /**
   * @brief 着色法线所在的本地坐标系
   */
  Frame Shading;
  Vec2 UV;
  Vec3 dPdU, dPdV;
  Vec3 dNdU, dNdV;
  /**
   * @brief 本地坐标系下的入射方向
   */
  Vec3 Wi;
  /**
   * @brief 相交的图元索引
   */
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
