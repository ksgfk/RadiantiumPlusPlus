#include <rad/offline/asset/triangle_model.h>

namespace Rad {

TriangleModel::TriangleModel(
    const Eigen::Vector3f* pos,
    UInt32 vertexCount,
    const UInt32* indices,
    UInt32 indexCount,
    const Eigen::Vector3f* normal,
    const Eigen::Vector2f* uv) {
  if (indexCount % 3 != 0) {
    throw RadException("无效的索引数量 {}, 必须是 3 的整数倍", indexCount);
  }
  _vertexCount = vertexCount;
  _indexCount = indexCount;
  _triangleCount = indexCount / 3;
  _position = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
  std::copy(pos, pos + vertexCount, _position.get());
  _indices = Share<UInt32[]>(new UInt32[indexCount]);
  std::copy(indices, indices + indexCount, _indices.get());
  if (normal != nullptr) {
    _normal = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
    std::copy(normal, normal + vertexCount, _normal.get());
  }
  if (uv != nullptr) {
    _uv = Share<Eigen::Vector2f[]>(new Eigen::Vector2f[vertexCount]);
    std::copy(uv, uv + vertexCount, _uv.get());
  }
}

}  // namespace Rad
