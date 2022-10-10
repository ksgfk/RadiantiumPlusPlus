#pragma once

#include "../common.h"
#include "interaction.h"

#include <vector>
#include <optional>

namespace Rad {

/**
 * @brief 场景负责管理渲染时需要用到的所有对象实例的生命周期, 包括
 * Shape, Accel, Light, Camera
 */
class Scene {
 public:
  Scene(
      Unique<Accel> accel,
      Unique<Camera> camera,
      std::vector<Unique<Light>>&& lights,
      std::vector<Unique<Medium>>&& mediums,
      Medium* globalMedium);

  const Camera& GetCamera() const { return *_camera; }
  Camera& GetCamera() { return *_camera; }
  const Light* GetLight(UInt32 index) const { return _lights[index].get(); }
  Light* GetLight(UInt32 index) { return _lights[index].get(); }
  const Light* GetEnvLight() const { return _envLight; }
  Light* GetEnvLight() { return _envLight; }

  /**
   * @brief 获取包围整个场景的包围盒
   */
  BoundingBox3 GetWorldBound() const;

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
   * @return std::pair<UInt32, Float> 光源索引, 采样光源的权重
   */
  std::pair<UInt32, Float> SampleLight(Float xi) const;
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
  Float PdfLightDirection(const Light* light, const Interaction& ref, const DirectionSampleResult& dsr) const;

  /**
   * @brief 根据碰撞点获取光源, 如果碰到是光源则返回光源对象, 如果什么都没碰到则返回环境光
   * 如果optional里什么都没说明获取不到任何光源
   */
  std::optional<Light*> GetLight(const SurfaceInteraction& si) const;
  /**
   * @brief 根据碰撞评估光源radiance
   */
  Spectrum EvalLight(const SurfaceInteraction& si) const;

  Medium* GetGlobalMedium() const { return _globalMedium; }

 private:
  Unique<Accel> _accel;
  Unique<Camera> _camera;
  std::vector<Unique<Light>> _lights;
  std::vector<Unique<Medium>> _mediums;
  Float _lightPdf;
  Light* _envLight = nullptr;
  Medium* _globalMedium = nullptr;
};

}  // namespace Rad
