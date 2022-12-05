#pragma once

#include "shape.h"

namespace Rad {

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
  MeshBase(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg);
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
