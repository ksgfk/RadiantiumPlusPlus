#pragma once

#include "../common.h"
#include "interaction.h"
#include "bsdf.h"

namespace Rad {

class Shape {
 public:
  virtual ~Shape() noexcept = default;

  Float SurfaceArea() const { return _surfaceArea; }

  /**
   * @brief 将形状提交到 embree 设备
   *
   * @param device embree设备指针
   * @param scene embree场景
   * @param id 图元的唯一id
   */
  virtual void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) const = 0;
  /**
   * @brief 从求交结果计算表面交点数据
   *
   * @param ray 求交射线
   * @param rec 求交结果
   * @return SurfaceInteraction
   */
  virtual SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) = 0;

  virtual PositionSampleResult SamplePosition(const Vector2& xi) const = 0;
  virtual Float PdfPosition(const PositionSampleResult& psr) const = 0;
  virtual DirectionSampleResult SampleDirection(const Interaction& ref, const Vector2& xi) const;
  virtual Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const;

  bool HasBsdf() const { return _bsdf != nullptr; }
  const Bsdf* GetBsdf() const { return _bsdf.get(); }
  Bsdf* GetBsdf() { return _bsdf.get(); }
  void AttachBsdf(Unique<Bsdf> bsdf) { _bsdf = std::move(bsdf); }

  bool IsLight() const { return _light != nullptr; }
  const Light* GetLight() const { return _light; }
  Light* GetLight() { return _light; }
  void AttachLight(Light* light) { _light = light; }

 protected:
  Float _surfaceArea;
  Unique<Bsdf> _bsdf;
  Light* _light = nullptr;
};

}  // namespace Rad
