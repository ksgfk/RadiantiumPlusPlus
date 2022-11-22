#include <rad/offline/asset/static_grid.h>

namespace Rad {

StaticGrid::StaticGrid(
    std::vector<Float32>&& data,
    const Eigen::Vector3i& size,
    UInt32 channel,
    const BoundingBox3& box)
    : _data(std::move(data)), _size(size), _channel(channel), _box(box) {
  if (_data.size() != _size.prod() * _channel) {
    throw RadArgumentException("大小不一致");
  }
  _max = -std::numeric_limits<Float>::infinity();
  for (auto v : _data) {
    _max = std::max(_max, v);
  }
}

}  // namespace Rad
