#pragma once

#include "../common.h"
#include "interaction.h"
#include "bsdf.h"
#include "../utility/distribution.h"

namespace Rad {

class RAD_EXPORT_API Shape {
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

/**
 * @brief 三角形网格
 * 求交工作交给Embree了, 不过还是写一下原理
 * 平面隐式定义:
 * (P - P') · N = 0
 * 其中 P' 是平面上一点, 如果一个点 P 带入公式成立, 则这个点在平面上
 * 射线的表达式为:
 * r(t) = O + t * D
 * 令 P = r(t)
 * 则 (O + t * D - P') · N = 0
 * 解出
 * t = ((P' - O) · N) / (D · N)
 * 还有更简单的方法, 叫做Moller Trumbore算法
 * 我们知道重心坐标插值一个点的公式:
 * P = (1 - b1 - b2)P0 + b1P1 + b2P2
 * 令 P = r(t)
 * O + t * D = (1 - b1 - b2)P0 + b1P1 + b2P2
 * 变形
 * O + t * D = P0 - b1P0 - b2P0 + b1P1 + b2P2
 * O + t * D = P0 + b1(P1 - P0) + b2(P2 - P0)
 * O - P0 = -(t * D) + b1(P1 - P0) + b2(P2 - P0)
 * 改写成矩阵形式
 *                        ↓(这是个三行一列的矩阵, 只能这样表示了233)
 *                       (t )
 * (-D, P1 - P0, P2 - P0)(b1) = (O - P0)
 *                       (b2)
 * 现在问题转化成了 AX=B 是否有解, 其中
 * A = (-D, E0, E1)
 * B = O - P0
 * E0 = P1 - P0
 * E1 = P2 - P0
 * 解方程可以用高斯消元法和克莱姆法则
 */
class RAD_EXPORT_API MeshBase : public Shape {
 public:
  MeshBase(BuildContext* ctx, const ConfigNode& cfg);
  virtual ~MeshBase() noexcept override = default;

  void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) const override;
  SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) override;
  PositionSampleResult SamplePosition(const Vector2& xi) const override;
  Float PdfPosition(const PositionSampleResult& psr) const override;

  static Float TriangleArea(const Vector3& p0, const Vector3& p1, const Vector3& p2);

 protected:
  void UpdateDistibution();

  std::shared_ptr<Eigen::Vector3f[]> _position;
  std::shared_ptr<Eigen::Vector3f[]> _normal;
  std::shared_ptr<Eigen::Vector2f[]> _uv;
  std::shared_ptr<UInt32[]> _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;

  Transform _toWorld;
  DiscreteDistribution1D _dist;
};

}  // namespace Rad
