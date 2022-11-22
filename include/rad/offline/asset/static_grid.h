#pragma once

#include "../common.h"

namespace Rad {

class StaticGrid {
 public:
  StaticGrid(
      std::vector<Float32>&& data,
      const Eigen::Vector3i& size,
      UInt32 channel,
      const BoundingBox3& box);

  Float32* GetData() { return _data.data(); }
  const Float32* GetData() const { return _data.data(); }
  const Eigen::Vector3i& GetSize() const { return _size; }
  const UInt32 GetChannelCount() const { return _channel; }
  const BoundingBox3& GetBoundingBox() const { return _box; }

 private:
  std::vector<Float32> _data;
  Eigen::Vector3i _size;
  UInt32 _channel;
  BoundingBox3 _box;
  Float32 _max;
};

}  // namespace Rad
