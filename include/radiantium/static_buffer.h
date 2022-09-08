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
/**
 * @brief 静态二维缓冲区, 建议只用来存平凡类型, 别存那些花里胡哨的
 * 假设2行3列
 * 索引: (0,0)(1,0) | (0,1)(1,1) | (0,2)(1,2)
 * 下标: 0    1     | 2    3     | 4    5     
 * 因此双层 foreach 的时候 row 在内层比较好, 可以紧密访问
 * 建议使用 StaticBuffer2D::GetRowSpan
 */
template <typename T>
class StaticBuffer2D {
 public:
  StaticBuffer2D(UInt32 width, UInt32 height) noexcept {
    _width = width;
    _height = height;
    _count = _width * _height;
    _buffer = std::make_unique<T[]>(_count);
  }

  StaticBuffer2D(const StaticBuffer2D& other) noexcept {
    _buffer = std::make_unique<T[]>(other._count);
    _width = other._width;
    _height = other._height;
    _count = other._count;
    std::copy(other._buffer.get(), other._buffer.get() + _count, _buffer.get());
  }

  StaticBuffer2D(StaticBuffer2D&& other) noexcept {
    _buffer = std::move(other._buffer);
    _width = other._width;
    _height = other._height;
    _count = other._count;
  }

  StaticBuffer2D& operator=(const StaticBuffer2D& other) noexcept {
    _buffer = std::make_unique<T[]>(other._count);
    _width = other._width;
    _height = other._height;
    _count = other._count;
    std::copy(other._buffer.get(), other._buffer.get() + _count, _buffer.get());
    return *this;
  }

  StaticBuffer2D& operator=(StaticBuffer2D&& other) noexcept {
    _buffer = std::move(other._buffer);
    _width = other._width;
    _height = other._height;
    _count = other._count;
    return *this;
  }

  /*属性*/
  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }
  /**
   * @brief 直接获取原始指针
   */
  T* Data() const { return _buffer.get(); }

  /*方法*/
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
  /**
   * @brief 获取一行数据, 第一个是这一行开头, 第二个是这一行有多长
   * 
   * @param y 第几列
   */
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
