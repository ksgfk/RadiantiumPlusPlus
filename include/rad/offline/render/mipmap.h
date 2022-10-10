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
    if ((image.Width() % 2 != 0) || (image.Height() % 2 != 0)) {
      Logger::Get()->warn("分辨率是2的整数倍的纹理才支持mipmap: {}, {}", image.Width(), image.Height());
      maxLevel = 0;
    }
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

  T EvalNearest(const Vector2& uv) const {
    return _mips[0].Eval(uv, FilterMode::Nearest, _wrap);
  }

  T EvalBilinear(const Vector2& uv) const {
    return _mips[0].Eval(uv, FilterMode::Bilinear, _wrap);
  }

  /**
   * @brief 三线性插值过滤
   */
  T EvalTrilinear(const Vector2& uv, const Vector2& duvdx, const Vector2& duvdy) const {
    Float x = duvdx.squaredNorm();
    Float y = duvdy.squaredNorm();
    Float width = std::sqrt(std::max(x, y));
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
      T low = EvalIsotropic(uv, down, FilterMode::Nearest);
      T high = EvalIsotropic(uv, down + 1, FilterMode::Nearest);
      auto calc = low * (1 - weight) + (high * weight);
      result = T(calc);
    }
    return result;
  }

  /**
   * @brief 各向异性过滤
   */
  T EvalAnisotropic(const Vector2& uv, const Vector2& duvdx, const Vector2& duvdy, Float anisoLevel) const {
    //照着pbrt抄的, 抄歪了, 近处的纹理上椭圆很明显
    Float x = duvdx.squaredNorm();
    Float y = duvdy.squaredNorm();
    Float big = std::sqrt(std::max(x, y));
    Float small = std::sqrt(std::min(x, y));
    Vector2 bigduv = x < y ? duvdy : duvdx;
    Vector2 smallduv = x < y ? duvdx : duvdy;
    if ((small * anisoLevel < big) && (small > 0)) {
      Float scale = big / (small * anisoLevel);
      smallduv *= scale;
      small *= scale;
    }
    if (small == 0) {
      return EvalIsotropic(uv, 0, FilterMode::Bilinear);
    }
    Float level = MaxLevel() - 1 + std::log2(std::max(small, Float(1e-8)));
    T result;
    if (level < 0) {
      result = EvalEWA(uv, 0, bigduv, smallduv);
    } else if (level >= MaxLevel() - 1) {
      result = EvalIsotropic(uv, MaxLevel() - 1, FilterMode::Bilinear);
    } else {
      size_t down = (size_t)std::floor(level);
      Float weight = level - down;
      T low = EvalEWA(uv, down, bigduv, smallduv);
      T high(0);
      if (weight != 0) {
        high = EvalEWA(uv, down + 1, bigduv, smallduv);
      }
      auto calc = low * (1 - weight) + (high * weight);
      result = T(calc);
    }
    return result;
  }

 private:
  T EvalIsotropic(const Vector2& uv, size_t level, FilterMode filter) const {
    size_t l = std::clamp(level, size_t(0), MaxLevel() - 1);
    T result = _mips[l].Eval(uv, filter, _wrap);
    return result;
  }

  T EvalEWA(const Vector2& uv_, size_t level, const Vector2& dst0_, const Vector2& dst1_) const {
    const TImage& map = _mips[level];
    Vector2 st(uv_.x() * (map.Width()), uv_.y() * (map.Height()));
    Vector2 dst0(dst0_.x() * map.Width(), dst0_.y() * map.Height());
    Vector2 dst1(dst1_.x() * map.Width(), dst1_.y() * map.Height());

    Float A = dst0[1] * dst0[1] + dst1[1] * dst1[1] + 1;
    Float B = -2 * (dst0[0] * dst0[1] + dst1[0] * dst1[1]);
    Float C = dst0[0] * dst0[0] + dst1[0] * dst1[0] + 1;
    Float invF = 1 / (A * C - B * B * Float(0.25));
    A *= invF;
    B *= invF;
    C *= invF;

    Float det = -B * B + 4 * A * C;
    Float invDet = 1 / det;
    Float uSqrt = std::sqrt(det * C), vSqrt = std::sqrt(det * A);
    Int32 s0 = (Int32)std::ceil(st[0] - 2 * invDet * uSqrt);
    Int32 s1 = (Int32)std::floor(st[0] + 2 * invDet * uSqrt);
    Int32 t0 = (Int32)std::ceil(st[1] - 2 * invDet * vSqrt);
    Int32 t1 = (Int32)std::floor(st[1] + 2 * invDet * vSqrt);

    const Vector2 invR(1 / Float(map.Width()), 1 / Float(map.Height()));
    T sum(0);
    Float sumWts = 0;
    for (int it = t0; it <= t1; ++it) {
      Float tt = it - st[1];
      for (int is = s0; is <= s1; ++is) {
        Float ss = is - st[0];
        Float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
        if (r2 < 1) {
          Float weight = std::exp(Float(-2) * r2) - std::exp(Float(-2));
          T value = map.Eval(Vector2(is, it).cwiseProduct(invR), FilterMode::Nearest, _wrap);
          auto weighted = value * weight;
          sum += T(weighted);
          sumWts += weight;
        }
      }
    }
    auto result = sum / sumWts;
    return T(result);
  }

  std::vector<TImage> _mips;
  WrapMode _wrap;
};

}  // namespace Rad
