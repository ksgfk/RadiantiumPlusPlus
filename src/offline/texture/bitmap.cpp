#include <rad/offline/render/texture.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/render/mipmap.h>

#include <type_traits>

namespace Rad {

/**
 * @brief 图片纹理, 没啥好说的, 支持双线性过滤
 */
template <typename T>
class Bitmap final : public Texture<T> {
 public:
  Bitmap(BuildContext* ctx, const ConfigNode& cfg) : Texture<T>() {
    std::string assetName = cfg.Read<std::string>("asset_name");
    ImageAsset* imageAsset = ctx->GetImage(assetName);
    Share<BlockBasedImage<T>> image;
    if constexpr (std::is_same_v<T, Color>) {
      image = imageAsset->ToImageRgb();
    } else if constexpr (std::is_same_v<T, Float32>) {
      image = imageAsset->ToImageGray();
    } else {
      throw RadInvalidOperationException("unknown type");
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

    this->_width = image->Width();
    this->_height = image->Height();

    if (!enableMipMap && maxLevel < 0) {
      Logger::Get()->warn("mipmap not enable but input max level. set max level to 0");
      maxLevel = 0;
    }
    if (enableMipMap) {
      _mipmapRes = std::make_unique<MipMap<T>>(*image, maxLevel, _wrap);
      Logger::Get()->info("generate mipmap, level: {}", _mipmapRes->MaxLevel());
    } else {
      _rawImageRes = image;
    }
    _enableMipmap = enableMipMap;
  }
  ~Bitmap() noexcept override = default;

 protected:
  T EvalImpl(const SurfaceInteraction& si) const override {
    if (_enableMipmap) {
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
    } else {
      return _rawImageRes->Eval(si.UV, _filter, _wrap);
    }
  }

  T ReadImpl(UInt32 x, UInt32 y) const override {
    return _enableMipmap ? _mipmapRes->GetLevel(0).Read(x, y) : _rawImageRes->Read(x, y);
  }

 private:
  Unique<MipMap<T>> _mipmapRes;
  Share<BlockBasedImage<T>> _rawImageRes;
  bool _enableMipmap;
  FilterMode _filter;
  WrapMode _wrap;
  Float _anisoLevel;
};

}  // namespace Rad

RAD_FACTORY_TEXTURE_DECLARATION(Bitmap, "bitmap");
