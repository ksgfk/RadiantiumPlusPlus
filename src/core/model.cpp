#include "radiantium/logger.h"
#include "radiantium/radiantium.h"
#include <radiantium/model.h>

namespace rad {

TriangleModel::TriangleModel(std::vector<Vec3>&& pos,
                             std::vector<UInt32>&& indices,
                             std::vector<Vec3>&& normal,
                             std::vector<Vec2>&& uv) {
  if (indices.size() % 3 != 0) {
    GetLogger()->error("invild model indices {}", indices.size());
  }
  _pos = std::move(pos);
  _normal = std::move(normal);
  _uv = std::move(uv);
  _indices = std::move(indices);
}

TriangleModel::TriangleModel(const TriangleModel& other) {
  _pos = other._pos;
  _normal = other._normal;
  _uv = other._uv;
  _indices = other._indices;
}

TriangleModel::TriangleModel(TriangleModel&& other) noexcept {
  _pos = std::move(other._pos);
  _normal = std::move(other._normal);
  _uv = std::move(other._uv);
  _indices = std::move(other._indices);
}

}  // namespace rad
