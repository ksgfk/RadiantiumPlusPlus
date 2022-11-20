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
  Vector3 P = Vector3::Zero();
  /**
   * @brief 交点的法线方向
   */
  Vector3 N = Vector3::Zero();

  Ray SpawnRay(const Vector3& d) const;
  Ray SpawnRayTo(const Vector3& p) const;
  bool IsValid() const;
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
  /**
   * @brief 表面UV纹理坐标
   */
  Vector2 UV;
  /**
   * @brief 表面坐标对于纹理坐标的偏导数
   */
  Vector3 dPdU, dPdV;
  /**
   * @brief 表面法线对于纹理坐标的偏导数
   */
  Vector3 dNdU, dNdV;
  /**
   * @brief UV坐标对于屏幕空间坐标的偏导数
   */
  Vector2 dUVdX = Vector2::Zero(), dUVdY = Vector2::Zero();
  /**
   * @brief 本地坐标系下的入射方向, 如果这个碰撞点不可用(没碰到任何物体), 则表示入射方向的反方向
   */
  Vector3 Wi;

  SurfaceInteraction() = default;
  SurfaceInteraction(const PositionSampleResult& dsr);

  void ComputeShadingFrame();
  Vector3 ToWorld(const Vector3& v) const;
  Vector3 ToLocal(const Vector3& v) const;

  PositionSampleResult ToPsr() const;
  DirectionSampleResult ToDsr(const Interaction& ref) const;

  Bsdf* BSDF();
  Bsdf* BSDF(const RayDifferential& ray);
  Medium* GetMedium(const Ray& ray) const;
};

struct MediumInteraction : public Interaction {
  Medium* Medium = nullptr;
  Frame Shading;
  Vector3 Wi;
  Float MinT;
  Spectrum SigmaS, SigmaN, SigmaT, CombinedExtinction;

  Vector3 ToWorld(const Vector3& v) const;
  Vector3 ToLocal(const Vector3& v) const;
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
