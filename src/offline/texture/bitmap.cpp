#include <rad/offline/render/texture.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <type_traits>

namespace Rad {

enum class FilterMode {
  Nearest,
  Linear
};

enum class WrapMode {
  Repeat,
  Clamp
};

/**
 * @brief 图片纹理, 没啥好说的, 支持双线性过滤
 */
template <typename T>
class Bitmap final : public Texture<T> {
 public:
  Bitmap(BuildContext* ctx, const ConfigNode& cfg) : Texture<T>() {
    std::string assetName = cfg.Read<std::string>("asset_name");
    ImageAsset* imageAsset = ctx->GetImage(assetName);
    if constexpr (std::is_same_v<T, Color>) {
      _image = imageAsset->ToImageRgb();
    } else if constexpr (std::is_same_v<T, Float32>) {
      _image = imageAsset->ToImageGray();
    } else {
      throw RadInvalidOperationException("unknown type");
    }

    std::string filterStr = cfg.ReadOrDefault("filter", std::string("linear"));
    if (filterStr == "nearest") {
      _filter = FilterMode::Nearest;
    } else if (filterStr == "linear") {
      _filter = FilterMode::Linear;
    } else {
      Logger::Get()->info("unknwon filter: {}, use nearest", filterStr);
      _filter = FilterMode::Nearest;
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

    this->_width = _image->Width();
    this->_height = _image->Height();
  }
  ~Bitmap() noexcept override = default;

 protected:
  T EvalImpl(const SurfaceInteraction& si) const override {
    const Vector2& uv = si.UV;
    Float u = Wrap(uv.x());
    Float v = Wrap(uv.y());
    UInt32 width = _image->Width();
    UInt32 height = _image->Height();
    Float fu = u * (width - 1);
    Float fv = v * (height - 1);
    UInt32 pu = (UInt32)fu;
    UInt32 pv = (UInt32)fv;
    switch (_filter) {
      case FilterMode::Linear: {
        Int32 dpu = (fu > pu + 0.5f) ? 1 : -1;
        Int32 dpv = (fv > pv + 0.5f) ? 1 : -1;
        UInt32 apu = std::clamp(pu + dpu, 0u, width - 1);
        UInt32 apv = std::clamp(pv + dpv, 0u, height - 1);
        Float du = std::min(std::abs(fu - pu - Float(0.5)), Float(1));
        Float dv = std::min(std::abs(fv - pv - Float(0.5)), Float(1));
        T u0v0 = _image->Read(pu, pv);
        T u1v0 = _image->Read(apu, pv);
        T u0v1 = _image->Read(pu, apv);
        T u1v1 = _image->Read(apu, apv);
        auto v = (u0v0 * (1 - du) + u1v0 * du) * (1 - dv) + (u0v1 * (1 - du) + u1v1 * du) * dv;
        return T(v);
      }
      case FilterMode::Nearest:
      default:
        return _image->Read(pu, pv);
    }
  }

  T ReadImpl(UInt32 x, UInt32 y) const override {
    return _image->Read(x, y);
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

  Share<BlockBasedImage<T>> _image;
  FilterMode _filter;
  WrapMode _wrap;
};

}  // namespace Rad

RAD_FACTORY_TEXTURE_DECLARATION(Bitmap, "bitmap");
