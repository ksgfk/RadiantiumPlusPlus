#include <radiantium/asset.h>

#include <radiantium/radiantium.h>

#if defined(RAD_DEFINE_DEBUG)
#include <stdexcept>
#endif

namespace rad {

BlockBasedImage::BlockBasedImage(UInt32 width, UInt32 height, UInt32 depth)
    : _width(width), _height(height), _depth(depth) {
  UInt32 x = _width / BlockSize;
  UInt32 y = _height / BlockSize;
  if (x % BlockSize != 0) {
    x++;
  }
  if (y % BlockSize != 0) {
    y++;
  }
  UInt32 count = x * y;
  _data.resize(count);
  for (auto& block : _data) {
    block.Data.resize(BlockSize * BlockSize * _depth);
  }
  _widthBlockCount = x;
}

BlockBasedImage::BlockBasedImage(UInt32 width, UInt32 height, UInt32 depth, const Float* data)
    : BlockBasedImage(width, height, depth) {
  for (UInt32 y = 0; y < _height; y++) {
    for (UInt32 x = 0; x < _width; x++) {
      auto [block, index] = GetIndex(x, y);
      for (UInt32 z = 0; z < _depth; z++) {
        _data[block].Data[index + z] = data[(y * _width + x) * _depth + z];
      }
    }
  }
}

Float BlockBasedImage::Read1(UInt32 x, UInt32 y) const {
  auto [block, index] = GetIndex(x, y);
#ifdef RAD_DEBUG_MODE
  if (_depth != 1) {
    throw std::invalid_argument("channel not 1");
  }
#endif
  const auto& data = _data[block].Data;
  return data[index];
}

RgbSpectrum BlockBasedImage::Read3(UInt32 x, UInt32 y) const {
  auto [block, index] = GetIndex(x, y);
#ifdef RAD_DEBUG_MODE
  if (_depth != 3) {
    throw std::invalid_argument("channel not 3");
  }
#endif
  const auto& data = _data[block].Data;
  return RgbSpectrum(data[index], data[index + 1], data[index + 2]);
}

std::pair<UInt32, UInt32> BlockBasedImage::GetIndex(UInt32 x, UInt32 y) const {
#ifdef RAD_DEBUG_MODE
  if (x >= _width) {
    throw std::out_of_range("x");
  }
  if (y >= _height) {
    throw std::out_of_range("y");
  }
#endif
  UInt32 blockX = x / BlockSize;
  UInt32 blockY = y / BlockSize;
  UInt32 innerX = x % BlockSize;
  UInt32 innerY = y % BlockSize;
  UInt32 block = blockY * _widthBlockCount + blockX;
  UInt32 index = (innerY * _width + innerX) * _depth;
  return std::make_pair(block, index);
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::AssetType type) {
  switch (type) {
    case rad::AssetType::Model:
      os << "Model";
      break;
    case rad::AssetType::Image:
      os << "Image";
      break;
  }
  return os;
}
