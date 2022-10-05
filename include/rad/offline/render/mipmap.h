#pragma once

#include "../common.h"
#include "../asset/block_based_image.h"

#include <vector>
#include <cmath>

namespace Rad {

/**
 * @brief mipmap 用于解决纹理采样率不足造成的aliasing
 * 双线性过滤相比point filter效果已经好了很多, 但还是无法解决某些情况下, 比如采样覆盖面积过大, 而采样率不足时的信息丢失
 * 解决办法也很暴力, 使用超采样, 只要采样率够高, 而且基本覆盖掉采样区域, 那么这些信息就不存在丢失了, 显然, 这样做开销很大
 * 超采样做的实际上是在查询区间内信息并计算平均值, 我们实际需要一种方法来高效查询区间平均值
 * 二维前缀和是不是一个好办法? 确实, 前缀和的计算非常精确, 但问题也出在精确上, 如果图片太大, 前缀和需要的储存位数可能会不够
 * mipmap 相当于前缀和的弱化版, 它预计算了不同覆盖面积下的区域平均值, 仅付出比原图多了1/2的空间
 */
template <typename T, size_t BlockSizeT = 16>
class MipMap {
 public:
  using TImage = BlockBasedImage<T, BlockSizeT>;

  MipMap() noexcept = default;
  MipMap(const TImage& image, Int32 maxLevel, WrapMode wrap) {
    _wrap = wrap;
    // TODO:如果图像大小不是2的n次方, 需要重采样
    //计算mip最大层级
    UInt32 realMaxLevel;
    if (maxLevel < 0) {
      realMaxLevel = 1 + (UInt32)std::log2(std::max(image.Width(), image.Height()));
    } else {
      realMaxLevel = maxLevel;
    }
    _mips.reserve(realMaxLevel);
    //复制0层
    _mips.emplace_back(TImage(image.Width(), image.Height()));
    for (UInt32 j = 0; j < image.Height(); j++) {
      for (UInt32 i = 0; i < image.Width(); i++) {
        _mips[0].Write(i, j, image.Read(i, j));
      }
    }
    const FilterMode filter = FilterMode::Bilinear;
    //逐层计算
    for (UInt32 a = 1; a < realMaxLevel; a++) {
      UInt32 uCnt = std::max(UInt32(1), _mips[a - 1].Width() / 2);
      UInt32 vCnt = std::max(UInt32(1), _mips[a - 1].Height() / 2);
      _mips.emplace_back(TImage(uCnt, vCnt));
      const TImage& last = _mips[a - 1];
      TImage& map = _mips[a];
      for (UInt32 j = 0; j < map.Height(); j++) {
        for (UInt32 i = 0; i < map.Width(); i++) {
          //相当于box filter
          T u0v0 = last.Eval(i * 2, j * 2, filter, _wrap);
          T u1v0 = last.Eval(i * 2 + 1, j * 2, filter, _wrap);
          T u0v1 = last.Eval(i * 2, j * 2 + 1, filter, _wrap);
          T u1v1 = last.Eval(i * 2 + 1, j * 2 + 1, filter, _wrap);
          auto color = (u0v0 + u1v0 + u0v1 + u1v1) * Float(0.25);
          map.Write(i, j, T(color));
        }
      }
    }
  }

  const size_t MaxLevel() const { return _mips.size(); }

  const TImage& GetLevel(UInt32 level) const {
    return _mips[level];
  }

  /**
   * @brief 邻近过滤和双线性插值
   */
  T Eval(const Vector2& uv, FilterMode filter) const {
    return _mips[0].Eval(uv, filter, _wrap);
  }

  /**
   * @brief 三线性插值
   */
  T Eval(const Vector2& uv, Float width) const {
    // mipmap所有层级分辨率都是2^n, 因此覆盖范围对应层级只需要取对数, 数组下标从0开始, 还要减1
    Float level = MaxLevel() - 1 + std::log2(std::max(width, Float(1e-8)));
    T result;
    if (level < 0) {
      result = EvalIsotropic(uv, 0, FilterMode::Bilinear);
    } else if (level >= MaxLevel() - 1) {
      result = EvalIsotropic(uv, MaxLevel() - 1, FilterMode::Bilinear);
    } else {
      size_t down = (size_t)std::floor(level);
      Float weight = level - down;
      T low = EvalIsotropic(uv, down, FilterMode::Bilinear);
      T high = EvalIsotropic(uv, down + 1, FilterMode::Bilinear);
      auto calc = low * (1 - weight) + (high * weight);
      result = T(calc);
    }
    return result;
  }

 private:
  /**
   * @brief 评估各向同性mipmap值, 假设采样区域是个正方形
   */
  T EvalIsotropic(const Vector2& uv, size_t level, FilterMode filter) const {
    size_t l = std::clamp(level, size_t(0), MaxLevel() - 1);
    T result = _mips[l].Eval(uv, filter, _wrap);
    return result;
  }

  std::vector<TImage> _mips;
  WrapMode _wrap;
};

}  // namespace Rad
