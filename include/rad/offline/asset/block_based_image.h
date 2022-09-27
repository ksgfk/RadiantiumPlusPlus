#pragma once

#include "../common.h"

namespace Rad {

template <typename T, size_t Size = 16>
class BlockBasedImage {
  constexpr static int BlockSize = Size;
  constexpr static float InvBlockSize = 1.0f / BlockSize;
  constexpr static int BlockArea = BlockSize * BlockSize;
  constexpr static float InvBlockArea = 1.0f / BlockArea;

 public:
  BlockBasedImage(UInt32 width, UInt32 height) : _width(width), _height(height) {
    UInt32 x = width / BlockSize;
    UInt32 y = height / BlockSize;
    if (width % BlockSize != 0) {
      x++;
    }
    if (height % BlockSize != 0) {
      y++;
    }
    _data.resize(x * BlockSize, y * BlockSize);
    _blockCountX = x;
    _blockCountY = y;
    _invBlockCountX = Math::Rcp((Float32)_blockCountX);
  }

  BlockBasedImage(UInt32 width, UInt32 height, const T* data) : BlockBasedImage(width, height) {
    for (UInt32 j = 0; j < height; j++) {
      for (UInt32 i = 0; i < width; i++) {
        Read(i, j) = data[j * width + i];
      }
    }
  }

  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }

  T& Read(UInt32 x, UInt32 y) {
    auto [innerX, innerY, blockX, blockY] = GetIndex(x, y);
    return _data.block(blockX, blockY, BlockSize, BlockSize).coeffRef(innerX, innerY);
  }
  const T& Read(UInt32 x, UInt32 y) const {
    auto [innerX, innerY, blockX, blockY] = GetIndex(x, y);
    return _data.block(blockX, blockY, BlockSize, BlockSize).coeff(innerX, innerY);
  }

  void Write(UInt32 x, UInt32 y, const T& data) {
    auto [innerX, innerY, blockX, blockY] = GetIndex(x, y);
    _data.block(blockX, blockY, BlockSize, BlockSize).coeffRef(innerX, innerY) = data;
  }

  MatrixX<T> ToFlat() const {
    MatrixX<T> flat;
    flat.resize(_width, _height);
    for (UInt32 j = 0; j < _height; j++) {
      for (UInt32 i = 0; i < _width; i++) {
        flat(i, j) = Read(i, j);
      }
    }
    return flat;
  }

 private:
  std::tuple<UInt32, UInt32, UInt32, UInt32> GetIndex(UInt32 x, UInt32 y) const {
    UInt32 index = y * _width + x;
    UInt32 blockIndex = static_cast<UInt32>(index * InvBlockArea);
    UInt32 blockX = blockIndex % _blockCountX;
    UInt32 blockY = (UInt32)(blockIndex * _invBlockCountX);
    UInt32 innerIndex = index % BlockArea;
    UInt32 innerX = innerIndex % BlockSize;
    UInt32 innerY = static_cast<UInt32>(innerIndex * InvBlockSize);
    return std::make_tuple(innerX, innerY, blockX * BlockSize, blockY * BlockSize);
  }

  MatrixX<T> _data;
  UInt32 _width;
  UInt32 _height;
  UInt32 _blockCountX;
  UInt32 _blockCountY;
  Float32 _invBlockCountX;
};

}  // namespace Rad
