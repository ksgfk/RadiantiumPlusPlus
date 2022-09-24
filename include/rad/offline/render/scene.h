#pragma once

#include "../common.h"
#include "interaction.h"

#include <vector>

namespace Rad {

/**
 * @brief 场景负责管理渲染时需要用到的所有对象实例的生命周期, 包括
 * Shape, Accel, Light, Camera
 */
class Scene {
 public:
  Scene(Unique<Accel> accel, Unique<Camera> camera, std::vector<Unique<Light>>&& lights);

  const Camera& GetCamera() const { return *_camera; }
  Camera& GetCamera() { return *_camera; }
  const Light* GetLight(UInt32 index) const { return _lights[index].get(); }
  Light* GetLight(UInt32 index) { return _lights[index].get(); }

  /**
   * @brief shadow ray, 检查射线是否与场景中任何一个 shape 有交点
   */
  bool RayIntersect(const Ray& ray) const;
  /**
   * @brief 光线求交, 如果射线与场景中任何一个 shape 有交点, 就会返回true, 并且si返回交点数据
   */
  bool RayIntersect(const Ray& ray, SurfaceInteraction& si) const;
  /**
   * @brief 检查参考点与目标点之间是否有遮挡
   */
  bool IsOcclude(const Interaction& ref, const Vector3& p) const;

  /**
   * @brief 采样场景中的光源
   *
   * @return std::tuple<UInt32, Float, Float> 光源索引, 采样光源的权重, 样本重用
   */
  std::tuple<UInt32, Float, Float> SampleLight(Float xi) const;
  /**
   * @brief 采样光源的概率密度
   */
  Float PdfLight(UInt32 index) const;
  /**
   * @brief 给定世界空间中的一个参考点, 采样一个光源, 并采样参考点到光源上某个点的方向
   * 采样方向的概率密度定义在单位立体角上

   * @return std::tuple<Light*, DirectionSampleResult, Spectrum> 光源实例, 采样方向的结果, 沿采样方向的反方向发出的 radiance
   */
  std::tuple<Light*, DirectionSampleResult, Spectrum> SampleLightDirection(
      const Interaction& ref,
      Float sampleLight,
      const Vector2& xi) const;

 private:
  Unique<Accel> _accel;
  Unique<Camera> _camera;
  std::vector<Unique<Light>> _lights;
  Float _lightPdf;
};

}  // namespace Rad