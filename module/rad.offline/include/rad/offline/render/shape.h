#pragma once

#include <rad/offline/distribution.h>
#include <rad/offline/transform.h>
#include <rad/core/config_node.h>

#include "interaction.h"

namespace Rad {

/**
 * @brief 所有形状的抽象
 * 形状可以是显示也可以是隐式定义的，三角形网格是显示定义，球体、矩形是隐式定义
 */
class RAD_EXPORT_API Shape {
 public:
  virtual ~Shape() noexcept = default;

  /**
   * @brief 形状表面积
   */
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

  /**
   * @brief 采样形状表面的点
   * 概率密度定义在单位面积上
   */
  virtual PositionSampleResult SamplePosition(const Vector2& xi) const = 0;
  /**
   * @brief 计算采样光源上的点的概率密度
   */
  virtual Float PdfPosition(const PositionSampleResult& psr) const = 0;
  /**
   * @brief 给定世界空间中的一个参考点, 采样一个从参考点到光源的方向
   * 采样方向的概率密度定义在单位立体角上
   * 采样点相对于参考点来说最好是可见的, 这样可以让有效样本变多, 减小方差
   */
  virtual DirectionSampleResult SampleDirection(const Interaction& ref, const Vector2& xi) const;
  /**
   * @brief 计算从参考点到形状方向的概率密度
   */
  virtual Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const;

  /**
   * @brief 通过UV计算表面参数
   */
  virtual SurfaceInteraction EvalParamSurface(const Vector2& uv);

  bool HasBsdf() const { return _bsdf != nullptr; }
  const Bsdf* GetBsdf() const { return _bsdf; }
  Bsdf* GetBsdf() { return _bsdf; }
  /**
   * @brief BSDF实例的生命周期不由Shape管理, 但保证需要用到BSDF时, 它是可用的
   */
  void AttachBsdf(Bsdf* bsdf) { _bsdf = bsdf; }

  bool IsLight() const { return _light != nullptr; }
  const Light* GetLight() const { return _light; }
  Light* GetLight() { return _light; }
  /**
   * @brief Light的生命周期不由Shape管理, 但保证需要用到Light时, 它是可用的
   */
  void AttachLight(Light* light) { _light = light; }
  /**
   * @brief Medium的生命周期不由Shape管理, 但保证需要用到Medium时, 它是可用的
   */
  inline void AttachMedium(Medium* inMedium, Medium* outMedium) {
    _insideMedium = inMedium;
    _outsideMedium = outMedium;
  }
  /**
   * @brief 内外是否存在参与介质
   */
  bool HasMedia() const { return _insideMedium != nullptr || _outsideMedium != nullptr; }
  Medium* GetInsideMedium() const { return _insideMedium; }
  Medium* GetOutsideMedium() const { return _outsideMedium; }

 protected:
  Float _surfaceArea;
  Bsdf* _bsdf = nullptr;
  Light* _light = nullptr;
  Medium* _insideMedium = nullptr;
  Medium* _outsideMedium = nullptr;
};

}  // namespace Rad
