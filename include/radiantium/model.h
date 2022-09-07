#pragma once

#include "radiantium.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include <memory>

namespace rad {

class TriangleModel {
 public:
  TriangleModel();
  TriangleModel(const Eigen::Vector3f* pos,
                UInt32 vertexCount,
                const UInt32* indices,
                UInt32 indexCount,
                const Eigen::Vector3f* normal = nullptr,
                const Eigen::Vector2f* uv = nullptr);
  TriangleModel(const TriangleModel&);
  TriangleModel(TriangleModel&&) noexcept;
  TriangleModel& operator=(const TriangleModel&);
  TriangleModel& operator=(TriangleModel&&) noexcept;

  /* Properties */
  std::shared_ptr<Eigen::Vector3f[]> GetPosition() const;
  std::shared_ptr<Eigen::Vector3f[]> GetNormal() const;
  std::shared_ptr<Eigen::Vector2f[]> GetUV() const;
  std::shared_ptr<UInt32[]> GetIndices() const;
  UInt32 TriangleCount() const { return _triangleCount; }
  UInt32 VertexCount() const { return _vertexCount; }
  UInt32 IndexCount() const { return _indexCount; }
  bool HasNormal() const;
  bool HasUV() const;

 private:
  std::shared_ptr<Eigen::Vector3f[]> _position;
  std::shared_ptr<Eigen::Vector3f[]> _normal;
  std::shared_ptr<Eigen::Vector2f[]> _uv;
  std::shared_ptr<UInt32[]> _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;
};

}  // namespace rad
