#include <rad/offline/render/texture.h>

#include <rad/core/memory.h>
#include <rad/core/logger.h>
#include <rad/core/config_node.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>

using namespace Rad::Math;

namespace Rad {

/**
 * @brief mipmap 用于解决纹理采样率不足造成的aliasing
 * 双线性过滤相比point filter效果已经好了很多, 但还是无法解决某些情况下, 比如采样覆盖面积过大, 而采样率不足时的信息丢失
 * 解决办法也很暴力, 使用超采样, 只要采样率够高, 而且基本覆盖掉采样区域, 那么这些信息就不存在丢失了, 显然, 这样做开销很大
 * 超采样做的实际上是在查询区间内信息并计算平均值, 我们实际需要一种方法来高效查询区间平均值
 * 二维前缀和是不是一个好办法? 确实, 前缀和的计算非常精确, 但问题也出在精确上, 如果图片太大, 前缀和需要的储存位数可能会不够
 * mipmap 相当于前缀和的弱化版, 它预计算了不同覆盖面积下的区域平均值, 仅付出比原图多了1/2的空间
 * 但是基于mipmap的三线性过滤还有个问题，它假设了采样区间是一个正方形区域，这一假设在极端情况会失效
 * 各向异性过滤在纹理空间里按椭圆形采样，应该是效果最好的采样方法
 */
template <typename T>
class MipMap {
 public:
  MipMap(const Share<BlockArray2D<T>>& image, Int32 maxLevel, WrapMode wrap) {
    _wrap = wrap;
    //检查是否支持mipmap
    if (maxLevel != 0 && ((image->Width() % 2 != 0) || (image->Height() % 2 != 0))) {
      Logger::Get()->warn("only resolution is a multiple of 2 can gen mipmap: {}, {}", image->Width(), image->Height());
      maxLevel = 0;
    }
    //计算mip最大层级
    UInt32 realMaxLevel;
    if (maxLevel < 0) {
      realMaxLevel = 1 + (UInt32)std::log2(std::max(image->Width(), image->Height()));
    } else {
      realMaxLevel = maxLevel;
    }
    realMaxLevel = std::max(realMaxLevel, 1u);
    _mips.reserve(realMaxLevel);
    //复制0层
    _mips.emplace_back(image);
    //逐层计算
    for (UInt32 a = 1; a < realMaxLevel; a++) {
      UInt32 uCnt = std::max(UInt32(1), (UInt32)(_mips[a - 1]->Width() / 2));
      UInt32 vCnt = std::max(UInt32(1), (UInt32)(_mips[a - 1]->Height() / 2));
      _mips.emplace_back(std::make_shared<BlockArray2D<T>>(uCnt, vCnt));
      const auto& last = *_mips[a - 1];
      auto& map = *_mips[a];
      for (UInt32 j = 0; j < map.Height(); j++) {
        for (UInt32 i = 0; i < map.Width(); i++) {
          //相当于box filter
          T u0v0 = FilterBilinear(last, i * 2, j * 2);
          T u1v0 = FilterBilinear(last, i * 2 + 1, j * 2);
          T u0v1 = FilterBilinear(last, i * 2, j * 2 + 1);
          T u1v1 = FilterBilinear(last, i * 2 + 1, j * 2 + 1);
          auto color = (u0v0 + u1v0 + u0v1 + u1v1) * Float(0.25);
          map(i, j) = T(color);
        }
      }
    }
  }

  const size_t MaxLevel() const { return _mips.size(); }
  const BlockArray2D<T>& GetLevel(UInt32 level) const { return *_mips[level]; }
  UInt32 Width() const noexcept { return (UInt32)_mips[0]->Width(); }
  UInt32 Height() const noexcept { return (UInt32)_mips[0]->Height(); }

  /**
   * @brief 邻近过滤
   */
  T EvalNearest(const Vector2& uv) const {
    return FilterNearest(*_mips[0], uv);
  }

  /**
   * @brief 双线性插值过滤
   */
  T EvalBilinear(const Vector2& uv) const {
    return FilterBilinear(*_mips[0], uv);
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
  Float Wrap(Float v) const {
    switch (_wrap) {
      case WrapMode::Repeat:
        return std::clamp(v - std::floor(v), Float(0), Float(1));
      case WrapMode::Clamp:
      default:
        return std::clamp(v, Float(0), Float(1));
    }
  }

  Eigen::Vector2<UInt32> Wrap(const BlockArray2D<T>& img, const Eigen::Vector2<UInt32>& uv) const {
    switch (_wrap) {
      case WrapMode::Repeat:
        return Eigen::Vector2<UInt32>(uv.x() % img.Width(), uv.y() % img.Height());
      case WrapMode::Clamp:
      default: {
        UInt32 x = std::clamp(uv.x(), UInt32(0), (UInt32)img.Width() - 1);
        UInt32 y = std::clamp(uv.y(), UInt32(0), (UInt32)img.Height() - 1);
        return Eigen::Vector2<UInt32>(x, y);
      }
    }
  }

  T FilterBilinear(const BlockArray2D<T>& img, UInt32 u, UInt32 v) const {
    auto uv = Wrap(img, Eigen::Vector2<UInt32>(u, v));
    UInt32 pu = uv.x();
    UInt32 pv = uv.y();
    Float fu = pu + Float(1);
    Float fv = pv + Float(1);
    return FilterBilinear(img, pu, pv, fu, fv);
  }

  T FilterNearest(const BlockArray2D<T>& img, const Vector2& uv) const {
    Float u = Wrap(uv.x());
    Float v = Wrap(uv.y());
    UInt32 width = (UInt32)img.Width();
    UInt32 height = (UInt32)img.Height();
    Float fu = u * (width - 1);
    Float fv = v * (height - 1);
    UInt32 pu = (UInt32)fu;
    UInt32 pv = (UInt32)fv;
    return img(pu, pv);
  }

  T FilterBilinear(const BlockArray2D<T>& img, const Vector2& uv) const {
    Float u = Wrap(uv.x());
    Float v = Wrap(uv.y());
    UInt32 width = (UInt32)img.Width();
    UInt32 height = (UInt32)img.Height();
    Float fu = u * (width - 1);
    Float fv = v * (height - 1);
    UInt32 pu = (UInt32)fu;
    UInt32 pv = (UInt32)fv;
    return FilterBilinear(img, pu, pv, fu, fv);
  }

  T FilterBilinear(const BlockArray2D<T>& img, UInt32 pu, UInt32 pv, Float fu, Float fv) const {
    Int32 dpu = (fu > pu + 0.5f) ? 1 : -1;
    Int32 dpv = (fv > pv + 0.5f) ? 1 : -1;
    UInt32 apu = std::clamp(pu + dpu, 0u, (UInt32)img.Width() - 1);
    UInt32 apv = std::clamp(pv + dpv, 0u, (UInt32)img.Height() - 1);
    Float du = std::min(std::abs(fu - pu - Float(0.5)), Float(1));
    Float dv = std::min(std::abs(fv - pv - Float(0.5)), Float(1));
    T u0v0 = img(pu, pv);
    T u1v0 = img(apu, pv);
    T u0v1 = img(pu, apv);
    T u1v1 = img(apu, apv);
    auto v = (u0v0 * (1 - du) + u1v0 * du) * (1 - dv) + (u0v1 * (1 - du) + u1v1 * du) * dv;
    return T(v);
  }

  T EvalIsotropic(const Vector2& uv, size_t level, FilterMode filter) const {
    size_t l = std::clamp(level, size_t(0), MaxLevel() - 1);
    T result = FilterBilinear(*_mips[l], uv);
    return result;
  }

  T EvalEWA(const Vector2& uv_, size_t level, const Vector2& dst0_, const Vector2& dst1_) const {
    const BlockArray2D<T>& map = *_mips[level];
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
          T value = EvalNearest(Vector2(is, it).cwiseProduct(invR));
          auto weighted = value * weight;
          sum += T(weighted);
          sumWts += weight;
        }
      }
    }
    auto result = sum / sumWts;
    return T(result);
  }

  std::vector<Share<BlockArray2D<T>>> _mips;
  WrapMode _wrap;
};

template <typename T>
class Bitmap final : public Texture<T> {
 public:
  Bitmap(BuildContext* ctx, const ConfigNode& cfg) : Texture<T>() {
    std::string assetName = cfg.Read<std::string>("asset_name");
    const ImageAsset* imageAsset = ctx->GetAssetManager().Borrow<ImageAsset>(assetName);
    Share<BlockArray2D<T>> image;
    if constexpr (std::is_same_v<T, Color24f>) {
      image = imageAsset->ToBlockImageRgb();
    } else if constexpr (std::is_same_v<T, Float32>) {
      image = imageAsset->ToBlockImageMono();
    }

    std::string filterStr = cfg.ReadOrDefault("filter", std::string("bilinear"));
    if (filterStr == "nearest") {
      _filter = FilterMode::Nearest;
    } else if (filterStr == "bilinear") {
      _filter = FilterMode::Bilinear;
    } else if (filterStr == "trilinear") {
      _filter = FilterMode::Trilinear;
    } else if (filterStr == "anisotropic") {
      _filter = FilterMode::Anisotropic;
    } else {
      Logger::Get()->info("unknwon filter: {}, use bilinear", filterStr);
      _filter = FilterMode::Bilinear;
    }
    std::string wrapStr = cfg.ReadOrDefault("wrap", std::string("clamp"));
    if (wrapStr == "clamp") {
      _wrap = WrapMode::Clamp;
    } else if (wrapStr == "repeat") {
      _wrap = WrapMode::Repeat;
    } else {
      Logger::Get()->info("unknwon wrapper: {}, use clamp", wrapStr);
      _wrap = WrapMode::Clamp;
    }
    bool enableMipMap = _filter == FilterMode::Trilinear || _filter == FilterMode::Anisotropic;
    Int32 maxLevel = cfg.ReadOrDefault("max_level", enableMipMap ? -1 : 0);
    bool useAniso = _filter == FilterMode::Anisotropic;
    _anisoLevel = cfg.ReadOrDefault("anisotropic_level", useAniso ? Float(8) : 0);

    this->_width = (UInt32)image->Width();
    this->_height = (UInt32)image->Height();

    if (!enableMipMap && maxLevel < 0) {
      Logger::Get()->warn("mipmap not enable but input max level. set max level to 0");
      maxLevel = 0;
    }
    _mipmapRes = std::make_unique<MipMap<T>>(std::move(image), maxLevel, _wrap);
    if (maxLevel > 0) {
      Logger::Get()->info("generate mipmap, level: {}", _mipmapRes->MaxLevel());
    }
  }
  ~Bitmap() noexcept override = default;

 protected:
  T EvalImpl(const SurfaceInteraction& si) const override {
    switch (_filter) {
      case FilterMode::Nearest:
        return _mipmapRes->EvalNearest(si.UV);
      case FilterMode::Bilinear:
        return _mipmapRes->EvalBilinear(si.UV);
      case FilterMode::Trilinear:
        return _mipmapRes->EvalTrilinear(si.UV, si.dUVdX, si.dUVdY);
      case FilterMode::Anisotropic:
        return _mipmapRes->EvalAnisotropic(si.UV, si.dUVdX, si.dUVdY, _anisoLevel);
      default:
        return T(0);
    }
  }

  T ReadImpl(UInt32 x, UInt32 y) const override {
    return _mipmapRes->GetLevel(0)(x, y);
  }

 private:
  Unique<MipMap<T>> _mipmapRes;
  FilterMode _filter;
  WrapMode _wrap;
  Float _anisoLevel;
};

class BitmapFactory : public TextureFactory {
 public:
  BitmapFactory() : TextureFactory("bitmap") {}
  ~BitmapFactory() noexcept override = default;
  Unique<TextureRGB> CreateRgb(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Bitmap<Color24f>>(ctx, cfg);
  }
  Unique<TextureMono> CreateMono(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Bitmap<Float32>>(ctx, cfg);
  }
};

Unique<TextureFactory> _FactoryCreateBitmapFunc_() {
  return std::make_unique<BitmapFactory>();
}

}  // namespace Rad
