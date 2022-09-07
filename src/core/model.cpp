#include <radiantium/model.h>

namespace rad {

TriangleModel::TriangleModel() = default;

TriangleModel::TriangleModel(
    const Eigen::Vector3f* pos,
    UInt32 vertexCount,
    const UInt32* indices,
    UInt32 indexCount,
    const Eigen::Vector3f* normal,
    const Eigen::Vector2f* uv) {
  if (indexCount % 3 != 0) {
    logger::GetLogger()->error("invalid index count {}", indexCount);
  }
  _vertexCount = vertexCount;
  _indexCount = indexCount;
  _triangleCount = indexCount % 3;
  _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
  std::copy(pos, pos + vertexCount, _position.get());
  _indices = std::shared_ptr<UInt32[]>(new UInt32[indexCount]);
  std::copy(indices, indices + indexCount, _indices.get());
  if (normal != nullptr) {
    _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
    std::copy(normal, normal + vertexCount, _normal.get());
  }
  if (uv != nullptr) {
    _uv = std::shared_ptr<Eigen::Vector2f[]>(new Eigen::Vector2f[vertexCount]);
    std::copy(uv, uv + vertexCount, _uv.get());
  }
}

TriangleModel::TriangleModel(const TriangleModel& other) {
  _vertexCount = other._vertexCount;
  _indexCount = other._indexCount;
  _triangleCount = other._triangleCount;
  _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
  std::copy(other._position.get(), other._position.get() + _vertexCount, _position.get());
  _indices = std::shared_ptr<UInt32[]>(new UInt32[_indexCount]);
  std::copy(other._indices.get(), other._indices.get() + _indexCount, _indices.get());
  if (other._normal != nullptr) {
    _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
    std::copy(other._normal.get(), other._normal.get() + _vertexCount, _normal.get());
  }
  if (other._uv != nullptr) {
    _uv = std::shared_ptr<Eigen::Vector2f[]>(new Eigen::Vector2f[_vertexCount]);
    std::copy(other._uv.get(), other._uv.get() + _vertexCount, _uv.get());
  }
}

TriangleModel::TriangleModel(TriangleModel&& other) noexcept {
  _position = std::move(other._position);
  _normal = std::move(other._normal);
  _uv = std::move(other._uv);
  _indices = std::move(other._indices);
  _vertexCount = other._vertexCount;
  _indexCount = other._indexCount;
  _triangleCount = other._triangleCount;
}

TriangleModel& TriangleModel::operator=(const TriangleModel& other) {
  _vertexCount = other._vertexCount;
  _indexCount = other._indexCount;
  _triangleCount = other._triangleCount;
  _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
  std::copy(other._position.get(), other._position.get() + _vertexCount, _position.get());
  _indices = std::shared_ptr<UInt32[]>(new UInt32[_indexCount]);
  std::copy(other._indices.get(), other._indices.get() + _indexCount, _indices.get());
  if (other._normal != nullptr) {
    _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
    std::copy(other._normal.get(), other._normal.get() + _vertexCount, _normal.get());
  }
  if (other._uv != nullptr) {
    _uv = std::shared_ptr<Eigen::Vector2f[]>(new Eigen::Vector2f[_vertexCount]);
    std::copy(other._uv.get(), other._uv.get() + _vertexCount, _uv.get());
  }
  return *this;
}

TriangleModel& TriangleModel::operator=(TriangleModel&& other) noexcept {
  _position = std::move(other._position);
  _normal = std::move(other._normal);
  _uv = std::move(other._uv);
  _indices = std::move(other._indices);
  _vertexCount = other._vertexCount;
  _indexCount = other._indexCount;
  _triangleCount = other._triangleCount;
  return *this;
}

std::shared_ptr<Eigen::Vector3f[]> TriangleModel::GetPosition() const { return _position; }

std::shared_ptr<Eigen::Vector3f[]> TriangleModel::GetNormal() const { return _normal; }

std::shared_ptr<Eigen::Vector2f[]> TriangleModel::GetUV() const { return _uv; }

std::shared_ptr<UInt32[]> TriangleModel::GetIndices() const { return _indices; }

bool TriangleModel::HasNormal() const { return _normal != nullptr; }

bool TriangleModel::HasUV() const { return _uv != nullptr; }

}  // namespace rad
