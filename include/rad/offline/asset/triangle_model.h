#pragma once

#include "../common.h"

namespace Rad {

class RAD_EXPORT_API TriangleModel {
 public:
  TriangleModel(const Eigen::Vector3f* pos,
                UInt32 vertexCount,
                const UInt32* indices,
                UInt32 indexCount,
                const Eigen::Vector3f* normal = nullptr,
                const Eigen::Vector2f* uv = nullptr);

  Share<Eigen::Vector3f[]> GetPosition() const { return _position; }
  Share<Eigen::Vector3f[]> GetNormal() const { return _normal; }
  Share<Eigen::Vector2f[]> GetUV() const { return _uv; }
  Share<UInt32[]> GetIndices() const { return _indices; }
  UInt32 TriangleCount() const { return _triangleCount; }
  UInt32 VertexCount() const { return _vertexCount; }
  UInt32 IndexCount() const { return _indexCount; }
  bool HasNormal() const { return _normal != nullptr; }
  bool HasUV() const { return _uv != nullptr; }

 private:
  Share<Eigen::Vector3f[]> _position;
  Share<Eigen::Vector3f[]> _normal;
  Share<Eigen::Vector2f[]> _uv;
  Share<UInt32[]> _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;
};

}  // namespace Rad
