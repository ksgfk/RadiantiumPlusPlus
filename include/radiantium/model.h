#pragma once

#include "radiantium.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace rad {

class TriangleModel {
  TriangleModel(std::vector<Vec3>&& pos,
        std::vector<UInt32>&& indices,
        std::vector<Vec3>&& normal = {},
        std::vector<Vec2>&& uv = {});
  TriangleModel(const TriangleModel&);
  TriangleModel(TriangleModel&&) noexcept;

  const std::vector<Vec3>& GetPosition() const { return _pos; }
  const std::vector<Vec3>& GetNormal() const { return _normal; }
  const std::vector<Vec2>& GetUV() const { return _uv; }
  const std::vector<UInt32>& GetIndices() const { return _indices; }

 private:
  std::vector<Vec3> _pos;
  std::vector<Vec3> _normal;
  std::vector<Vec2> _uv;
  std::vector<UInt32> _indices;
};

}  // namespace rad
