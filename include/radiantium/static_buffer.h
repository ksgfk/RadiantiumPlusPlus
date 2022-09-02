#pragma once

#include "radiantium.h"

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
  StaticBuffer(uint32_t width, uint32_t height) noexcept {
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
    for (size_t i = 0; i < _count; i++) {
      _buffer[i] = other._buffer[i];
    }
  }

  StaticBuffer(StaticBuffer&& other) noexcept {
    _buffer = std::move(other._buffer);
    _width = other._width;
    _height = other._height;
    _count = other._count;
  }

  /* Properties */
  uint32_t Width() const { return _width; }
  uint32_t Height() const { return _height; }
  T* Data() const { return _buffer.get(); }

  /* Methods */
  uint32_t GetIndex(uint32_t x, uint32_t y) const {
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
  std::pair<T*, uint32_t> GetRowSpan(uint32_t y) const {
#ifdef RAD_DEBUG_MODE
    if (y >= _height) {
      throw std::out_of_range("y");
    }
#endif
    uint32_t start = y * _width;
    T* startPtr = &_buffer[start];
    return std::make_pair(startPtr, _width);
  }

  const T& operator[](uint32_t i) const {
#ifdef RAD_DEBUG_MODE
    if (i >= _count) {
      throw std::out_of_range("i");
    }
#endif
    return _buffer[i];
  }
  T& operator[](uint32_t i) {
#ifdef RAD_DEBUG_MODE
    if (i >= _count) {
      throw std::out_of_range("i");
    }
#endif
    return _buffer[i];
  }
  const T& operator()(uint32_t x, uint32_t y) const {
    uint32_t index = GetIndex(x, y);
    return _buffer[index];
  }
  T& operator()(uint32_t x, uint32_t y) {
    uint32_t index = GetIndex(x, y);
    return _buffer[index];
  }

 private:
  uint32_t _width;
  uint32_t _height;
  uint32_t _count;
  std::unique_ptr<T[]> _buffer;
};

}  // namespace rad
