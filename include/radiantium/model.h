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
  TriangleModel(std::vector<Eigen::Vector3f>&& pos,
                std::vector<UInt32>&& indices,
                std::vector<Eigen::Vector3f>&& normal = {},
                std::vector<Eigen::Vector2f>&& uv = {});
  TriangleModel(const TriangleModel&);
  TriangleModel(TriangleModel&&) noexcept;
  TriangleModel& operator=(const TriangleModel&);
  TriangleModel& operator=(TriangleModel&&) noexcept;

  /* Properties */
  const std::vector<Eigen::Vector3f>& GetPosition() const;
  const std::vector<Eigen::Vector3f>& GetNormal() const;
  const std::vector<Eigen::Vector2f>& GetUV() const;
  const std::vector<UInt32>& GetIndices() const;
  const UInt32 TriangleCount() const { return _triangleCount; }
  const size_t VertexCount() const;
  const size_t IndexCount() const;
  bool HasNormal() const;
  bool HasUV() const;

  Eigen::Vector3f* GetPositionPtr() const;
  Eigen::Vector3f* GetNormalPtr() const;
  Eigen::Vector2f* GetUVPtr() const;
  UInt32* GetIndicePtr() const;

 private:
  std::unique_ptr<std::vector<Eigen::Vector3f>> _pos;
  std::unique_ptr<std::vector<Eigen::Vector3f>> _normal;
  std::unique_ptr<std::vector<Eigen::Vector2f>> _uv;
  std::unique_ptr<std::vector<UInt32>> _indices;
  UInt32 _triangleCount;
};

}  // namespace rad
