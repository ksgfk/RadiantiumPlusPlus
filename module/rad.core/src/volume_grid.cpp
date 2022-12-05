#include <rad/core/volume_grid.h>

namespace Rad {

VolumeGrid::VolumeGrid(
    std::vector<Float32>&& data,
    const Eigen::Vector3i& size,
    UInt32 channel,
    const BoundingBox3f& box)
    : _data(std::move(data)), _size(size), _channel(channel), _box(box) {
  if (_data.size() != _size.prod() * _channel) {
    throw RadArgumentException("inconsistent size");
  }
  _max = -std::numeric_limits<Float32>::infinity();
  for (auto v : _data) {
    _max = std::max(_max, v);
  }
}

}  // namespace Rad
