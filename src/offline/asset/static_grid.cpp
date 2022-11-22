#include <rad/offline/asset/static_grid.h>

namespace Rad {

StaticGrid::StaticGrid(
    std::unique_ptr<Float32[]> data,
    const Eigen::Vector3i& size,
    UInt32 channel,
    const BoundingBox3& box)
    : _data(std::move(data)), _size(size), _channel(channel), _box(box) {
  UInt32 all = _size.prod() * _channel;
  _max = -std::numeric_limits<Float>::infinity();
  for (UInt32 i = 0; i < all; i++) {
    _max = std::max(_max, data[i]);
  }
}

}  // namespace Rad
