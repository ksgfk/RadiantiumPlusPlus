#pragma once

#include "radiantium.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#ifdef RAD_DEBUG_MODE
#include <exception>
#endif

namespace rad {

template <typename T>
class StaticBuffer {
 public:
  StaticBuffer(UInt32 width, UInt32 height) noexcept {
    _width = width;
    _height = height;
    _count = _width * _height;
    _buffer = std::make_unique<T[]>(_count);
  }

  StaticBuffer(const StaticBuffer& other) noexcept {
    _buffer = std::make_unique<T[]>(other._count);
    _width = other._width;
    _height = other._height;
    _count = other._count;
    std::copy(other._buffer.get(), other._buffer.get() + _count, _buffer.get());
  }

  StaticBuffer(StaticBuffer&& other) noexcept {
    _buffer = std::move(other._buffer);
    _width = other._width;
    _height = other._height;
    _count = other._count;
  }

  /* Properties */
  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }
  T* Data() const { return _buffer.get(); }

  /* Methods */
  UInt32 GetIndex(UInt32 x, UInt32 y) const {
#ifdef RAD_DEBUG_MODE
    if (x >= _width) {
      throw std::out_of_range("x");
    }
    if (y >= _height) {
      throw std::out_of_range("y");
    }
#endif
    return y * _width + x;
  }
  std::pair<T*, UInt32> GetRowSpan(UInt32 y) const {
#ifdef RAD_DEBUG_MODE
    if (y >= _height) {
      throw std::out_of_range("y");
    }
#endif
    UInt32 start = y * _width;
    T* startPtr = &_buffer[start];
    return std::make_pair(startPtr, _width);
  }

  const T& operator[](UInt32 i) const {
#ifdef RAD_DEBUG_MODE
    if (i >= _count) {
      throw std::out_of_range("i");
    }
#endif
    return _buffer[i];
  }
  T& operator[](UInt32 i) {
#ifdef RAD_DEBUG_MODE
    if (i >= _count) {
      throw std::out_of_range("i");
    }
#endif
    return _buffer[i];
  }
  const T& operator()(UInt32 x, UInt32 y) const {
    UInt32 index = GetIndex(x, y);
    return _buffer[index];
  }
  T& operator()(UInt32 x, UInt32 y) {
    UInt32 index = GetIndex(x, y);
    return _buffer[index];
  }

 private:
  UInt32 _width;
  UInt32 _height;
  UInt32 _count;
  std::unique_ptr<T[]> _buffer;
};

}  // namespace rad
