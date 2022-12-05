#pragma once

#include "types.h"

namespace Rad {

/**
 * @brief 基于三角形的网格模型数据结构
 */
class RAD_EXPORT_API TriangleModel {
 public:
  TriangleModel(
      const Eigen::Vector3f* pos,
      UInt32 vertexCount,
      const UInt32* indices,
      UInt32 indexCount,
      const Eigen::Vector3f* normal = nullptr,
      const Eigen::Vector2f* uv = nullptr,
      const Eigen::Vector3f* tangent = nullptr);

  Share<Eigen::Vector3f[]> GetPosition() const { return _position; }
  Share<Eigen::Vector3f[]> GetNormal() const { return _normal; }
  Share<Eigen::Vector2f[]> GetUV() const { return _uv; }
  Share<Eigen::Vector3f[]> GetTangent() const { return _tangent; }
  Share<UInt32[]> GetIndices() const { return _indices; }
  UInt32 TriangleCount() const { return _triangleCount; }
  UInt32 VertexCount() const { return _vertexCount; }
  UInt32 IndexCount() const { return _indexCount; }
  bool HasNormal() const { return _normal != nullptr; }
  bool HasUV() const { return _uv != nullptr; }
  bool HasTangent() const { return _tangent != nullptr; }

  static TriangleModel CreateSphere(Float32 radius, Int32 numberSlices);
  static TriangleModel CreateCube(Float32 halfExtend);
  static TriangleModel CreateQuad(Float32 halfExtend);
  static TriangleModel CreateCylinder(Float32 halfExtend, Float32 radius, Int32 numberSlices);
  static TriangleModel CreateCylinder(
      Float32 bottomRadius,
      Float32 topRadius,
      Float32 height,
      UInt32 sliceCount,
      UInt32 stackCount);
  static TriangleModel CreateGrid(Float32 width, Float32 depth, UInt32 m, UInt32 n);

 private:
  Share<Eigen::Vector3f[]> _position;
  Share<Eigen::Vector3f[]> _normal;
  Share<Eigen::Vector2f[]> _uv;
  Share<Eigen::Vector3f[]> _tangent;
  Share<UInt32[]> _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;
};

}  // namespace Rad
