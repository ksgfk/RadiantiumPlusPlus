#pragma once

#include "../common.h"
#include "interaction.h"

#include <type_traits>

namespace Rad {

class TextureBase {
 public:
  virtual ~TextureBase() noexcept = default;

  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }

 protected:
  UInt32 _width;
  UInt32 _height;
};

/**
 * @brief 纹理, 支持RGB与Gray两种采样格式
 * 缓存常量颜色
 */
template <typename T>
class Texture : public TextureBase {
  static_assert(std::is_same_v<T, Color> || std::is_same_v<T, Float32>, "T must be color or float32");

 public:
  Texture() : _isConstColor(false) {}
  Texture(const T& color) : _isConstColor(true), _constColor(color) {
    this->_width = 1;
    this->_height = 1;
  }
  virtual ~Texture() noexcept = default;

  /**
   * @brief 根据交点评估纹理颜色
   */
  T Eval(const SurfaceInteraction& si) const {
    if (_isConstColor) {
      return _constColor;
    }
    return EvalImpl(si);
  }

  /**
   * @brief 直接读取纹理的值
   */
  T Read(UInt32 x, UInt32 y) const {
    if (_isConstColor) {
      return _constColor;
    }
    return Read(x, y);
  }

 protected:
  virtual T EvalImpl(const SurfaceInteraction& si) const {
    throw RadInvalidOperationException("texture is not const but hasn't impl eval func");
  }
  virtual T ReadImpl(UInt32 x, UInt32 y) const {
    throw RadInvalidOperationException("texture is not const but hasn't impl read func");
  }

 private:
  bool _isConstColor;
  T _constColor;
};

using TextureRGB = Texture<Color>;
using TextureGray = Texture<Float32>;

}  // namespace Rad
