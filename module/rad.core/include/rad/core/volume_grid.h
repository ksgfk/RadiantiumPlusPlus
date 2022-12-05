#pragma once

#include "types.h"

#include <vector>

namespace Rad {

/**
 * @brief 体积网格数据的抽象
 */
class RAD_EXPORT_API VolumeGrid {
 public:
  VolumeGrid(
      std::vector<Float32>&& data,
      const Eigen::Vector3i& size,
      UInt32 channel,
      const BoundingBox3f& box);

  std::vector<Float32>& GetData() { return _data; }
  const std::vector<Float32>& GetData() const { return _data; }
  const Eigen::Vector3i& GetSize() const { return _size; }
  const UInt32 GetChannelCount() const { return _channel; }
  const BoundingBox3f& GetBoundingBox() const { return _box; }
  const Float32 GetMaxValue() const { return _max; }

 private:
  std::vector<Float32> _data;
  Eigen::Vector3i _size;
  UInt32 _channel;
  BoundingBox3f _box;
  Float32 _max;
};

}  // namespace Rad
