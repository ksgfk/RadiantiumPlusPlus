#pragma once

#include <cstddef>
#include <vector>
#include <cmath>

#include "math_ext.h"

namespace Rad::Memory {

void* AlignedAlloc(std::size_t alignment, std::size_t size);
void AlignedFree(void* ptr);

/**
 * @brief https://pbr-book.org/3ed-2018/Utilities/Memory_Management#Blocked2DArrays
 * 居然可以用位运算...长见识了
 */
template <typename T, size_t BlockSizeT = 16>
class BlockArray2D {
  static_assert(Rad::Math::IsPowOf2(BlockSizeT), "BlockSize must be pow of 2");

 public:
  constexpr BlockArray2D() noexcept = default;
  constexpr BlockArray2D(std::size_t u, std::size_t v, const T* data = nullptr) noexcept
      : _logBlockSize((std::size_t)std::log2(BlockSizeT)), _u(u), _v(v), _blockU(RoundUp(_u) >> _logBlockSize) {
    std::size_t allSize = RoundUp(_u) * RoundUp(_v);
    _data.resize(allSize);
  }

  constexpr std::size_t Width() const noexcept { return _u; }
  constexpr std::size_t Height() const noexcept { return _v; }
  constexpr std::size_t BlockSize() const noexcept { return BlockSizeT; }
  /**
   * @brief 将输入x向上补齐为BlockSize的倍数
   */
  constexpr std::size_t RoundUp(std::size_t x) const noexcept {
    return (x + BlockSize() - 1) & ~(BlockSize() - 1);
  }
  /**
   * @brief 数组索引所在的块
   */
  constexpr std::size_t BlockIndex(std::size_t index) const noexcept { return index >> _logBlockSize; }
  /**
   * @brief 数组索引在块内的偏移量
   */
  constexpr std::size_t Offset(std::size_t a) const noexcept { return (a & (BlockSize() - 1)); }
  /**
   * @brief 获取索引所在地址下标
   */
  constexpr std::size_t GetIndex(std::size_t u, std::size_t v) const noexcept {
    std::size_t bu = BlockIndex(u), bv = BlockIndex(v);
    std::size_t ou = Offset(u), ov = Offset(v);
    std::size_t offset = (BlockSize() * BlockSize()) * (_blockU * bv + bu) + (BlockSize() * ov + ou);
    return offset;
  }

  T& operator()(std::size_t u, std::size_t v) {
#if defined(RAD_DEFINE_DEBUG)
    if (u >= _u) {
      throw RadOutOfRangeException("{} 超出索引范围", "u");
    }
    if (v >= _v) {
      throw RadOutOfRangeException("{} 超出索引范围", "v");
    }
#endif
    std::size_t i = GetIndex(u, v);
    return _data[i];
  }
  const T& operator()(std::size_t u, std::size_t v) const {
#if defined(RAD_DEFINE_DEBUG)
    if (u >= _u) {
      throw RadOutOfRangeException("{} 超出索引范围", "u");
    }
    if (v >= _v) {
      throw RadOutOfRangeException("{} 超出索引范围", "v");
    }
#endif
    std::size_t i = GetIndex(u, v);
    return _data[i];
  }

 private:
  std::vector<T> _data;
  const std::size_t _logBlockSize, _u, _v, _blockU;
};

}  // namespace Rad::Memory
