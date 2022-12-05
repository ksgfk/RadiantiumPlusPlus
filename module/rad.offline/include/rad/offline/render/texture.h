#pragma once

#include "interaction.h"

#include <type_traits>

namespace Rad {

enum class FilterMode {
  Nearest,
  Bilinear,
  Trilinear,
  Anisotropic
};

enum class WrapMode {
  Repeat,
  Clamp
};

/**
 * @brief 二维纹理，实际上是一个二维数组，记录了颜色数据。纹理可以是RGB也可以是单色的
 * 二维纹理使用UV坐标，范围[0,1]^2
 * 超出范围也是合法的
 */
class RAD_EXPORT_API TextureBase {
 public:
  virtual ~TextureBase() noexcept = default;

  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }

 protected:
  UInt32 _width;
  UInt32 _height;
};

/**
 * @brief 纹理, 支持RGB与单色两种采样格式
 * 缓存常量颜色
 */
template <typename T>
class Texture : public TextureBase {
  static_assert(std::is_same_v<T, Color24f> || std::is_same_v<T, Float32>, "T must be color or float32");

 public:
  Texture() : _isConstColor(false) {}
  Texture(const T& color) : _isConstColor(true), _constColor(color) {
    this->_width = 1;
    this->_height = 1;
  }
  virtual ~Texture() noexcept = default;

  /**
   * @brief 根据交点评估纹理颜色，一般来说使用si.UV参数
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
    return ReadImpl(x, y);
  }

  bool IsConstColor() const {
    return _isConstColor;
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

using TextureRGB = Texture<Color24f>;
using TextureMono = Texture<Float32>;

}  // namespace Rad
