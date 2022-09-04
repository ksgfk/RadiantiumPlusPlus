#include <radiantium/model.h>

namespace rad {

TriangleModel::TriangleModel(std::vector<Vec3>&& pos,
                             std::vector<UInt32>&& indices,
                             std::vector<Vec3>&& normal,
                             std::vector<Vec2>&& uv) {
  if (indices.size() % 3 != 0) {
    GetLogger()->error("invild model indices {}", indices.size());
  }
  if (normal.size() > 0 && normal.size() != pos.size()) {
    GetLogger()->error("invild normal {}", normal.size());
  }
  if (uv.size() > 0 && uv.size() != pos.size()) {
    GetLogger()->error("invild uv {}", uv.size());
  }
  _pos = std::make_unique<std::vector<Vec3>>(std::move(pos));
  _normal = std::make_unique<std::vector<Vec3>>(std::move(normal));
  _uv = std::make_unique<std::vector<Vec2>>(std::move(uv));
  _indices = std::make_unique<std::vector<UInt32>>(std::move(indices));
}

TriangleModel::TriangleModel(const TriangleModel& other) {
  _pos = std::make_unique<std::vector<Vec3>>(*other._pos);
  _normal = std::make_unique<std::vector<Vec3>>(*other._normal);
  _uv = std::make_unique<std::vector<Vec2>>(*other._uv);
  _indices = std::make_unique<std::vector<UInt32>>(*other._indices);
}

TriangleModel::TriangleModel(TriangleModel&& other) noexcept {
  _pos = std::move(other._pos);
  _normal = std::move(other._normal);
  _uv = std::move(other._uv);
  _indices = std::move(other._indices);
}

TriangleModel& TriangleModel::operator=(const TriangleModel& other) {
  _pos = std::make_unique<std::vector<Vec3>>(*other._pos);
  _normal = std::make_unique<std::vector<Vec3>>(*other._normal);
  _uv = std::make_unique<std::vector<Vec2>>(*other._uv);
  _indices = std::make_unique<std::vector<UInt32>>(*other._indices);
  return *this;
}

TriangleModel& TriangleModel::operator=(TriangleModel&& other) noexcept {
  _pos = std::move(other._pos);
  _normal = std::move(other._normal);
  _uv = std::move(other._uv);
  _indices = std::move(other._indices);
  return *this;
}

const std::vector<Vec3>& TriangleModel::GetPosition() const { return *_pos; }

const std::vector<Vec3>& TriangleModel::GetNormal() const { return *_normal; }

const std::vector<Vec2>& TriangleModel::GetUV() const { return *_uv; }

const std::vector<UInt32>& TriangleModel::GetIndices() const { return *_indices; }

const size_t TriangleModel::VertexCount() const { return _pos->size(); }

const size_t TriangleModel::IndexCount() const { return _indices->size(); }

bool TriangleModel::HasNormal() const { return _normal->size() > 0; }

bool TriangleModel::HasUV() const { return _uv->size() > 0; }

Vec3* TriangleModel::GetPositionPtr() const { return _pos.get()->data(); }

Vec3* TriangleModel::GetNormalPtr() const {
  return _normal->size() > 0 ? _normal.get()->data() : nullptr;
}

Vec2* TriangleModel::GetUVPtr() const {
  return _uv->size() > 0 ? _uv.get()->data() : nullptr;
}

UInt32* TriangleModel::GetIndicePtr() const { return _indices.get()->data(); }

}  // namespace rad
