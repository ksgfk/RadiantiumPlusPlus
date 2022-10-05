#pragma once

#include "../common.h"
#include "../memory.h"

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

template <typename T, size_t BlockSizeT = 16>
class BlockBasedImage {
 public:
  BlockBasedImage(UInt32 width, UInt32 height) : _arr(width, height, nullptr) {}
  BlockBasedImage(UInt32 width, UInt32 height, const T* data) : _arr(width, height, data) {}

  UInt32 Width() const noexcept { return (UInt32)_arr.Width(); }
  UInt32 Height() const noexcept { return (UInt32)_arr.Height(); }

  T& Read(UInt32 x, UInt32 y) { return _arr(x, y); }
  const T& Read(UInt32 x, UInt32 y) const { return _arr(x, y); }
  void Write(UInt32 x, UInt32 y, const T& data) { _arr(x, y) = data; }

  T Eval(UInt32 u, UInt32 v, FilterMode filter, WrapMode wrap) const {
    auto uv = Wrap(Eigen::Vector2<UInt32>(u, v), wrap);
    UInt32 pu = uv.x();
    UInt32 pv = uv.y();
    return Filter(pu, pv, pu + Float(1), pv + Float(1), filter);
  }

  T Eval(const Vector2& uv, FilterMode filter, WrapMode wrap) const {
    Float u = Wrap(uv.x(), wrap);
    Float v = Wrap(uv.y(), wrap);
    UInt32 width = Width();
    UInt32 height = Height();
    Float fu = u * (width - 1);
    Float fv = v * (height - 1);
    UInt32 pu = (UInt32)fu;
    UInt32 pv = (UInt32)fv;
    return Filter(pu, pv, fu, fv, filter);
  }

 private:
  static Float Wrap(Float v, WrapMode wrap) {
    switch (wrap) {
      case WrapMode::Repeat:
        return std::clamp(v - std::floor(v), Float(0), Float(1));
      case WrapMode::Clamp:
      default:
        return std::clamp(v, Float(0), Float(1));
    }
  }

  Eigen::Vector2<UInt32> Wrap(const Eigen::Vector2<UInt32>& uv, WrapMode wrap) const {
    switch (wrap) {
      case WrapMode::Repeat:
        return Eigen::Vector2<UInt32>(uv.x() % _arr.Width(), uv.y() % _arr.Height());
      case WrapMode::Clamp:
      default: {
        UInt32 x = std::clamp(uv.x(), UInt32(0), (UInt32)_arr.Width() - 1);
        UInt32 y = std::clamp(uv.y(), UInt32(0), (UInt32)_arr.Height() - 1);
        return Eigen::Vector2<UInt32>(x, y);
      }
    }
  }

  T Filter(UInt32 pu, UInt32 pv, Float fu, Float fv, FilterMode filter) const {
    switch (filter) {
      // https://pbr-book.org/3ed-2018/Texture/Texture_Interface_and_Basic_Textures#BilinearInterpolation
      case FilterMode::Anisotropic:
      case FilterMode::Trilinear:
      case FilterMode::Bilinear: {
        Int32 dpu = (fu > pu + 0.5f) ? 1 : -1;
        Int32 dpv = (fv > pv + 0.5f) ? 1 : -1;
        UInt32 apu = std::clamp(pu + dpu, 0u, Width() - 1);
        UInt32 apv = std::clamp(pv + dpv, 0u, Height() - 1);
        Float du = std::min(std::abs(fu - pu - Float(0.5)), Float(1));
        Float dv = std::min(std::abs(fv - pv - Float(0.5)), Float(1));
        T u0v0 = Read(pu, pv);
        T u1v0 = Read(apu, pv);
        T u0v1 = Read(pu, apv);
        T u1v1 = Read(apu, apv);
        auto v = (u0v0 * (1 - du) + u1v0 * du) * (1 - dv) + (u0v1 * (1 - du) + u1v1 * du) * dv;
        return T(v);
      }
      case FilterMode::Nearest:
      default:
        return Read(pu, pv);
    }
  }

  Memory::BlockArray2D<T, BlockSizeT> _arr;
};

}  // namespace Rad
