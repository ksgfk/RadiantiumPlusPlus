#include <radiantium/texture.h>

#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>
#include <radiantium/spectrum.h>
#include <radiantium/static_buffer.h>
#include <radiantium/math_ext.h>

#include <memory>
#include <cmath>

using namespace rad::math;

namespace rad {

enum class FilterMode {
  Nearest,
  Linear
};

enum class WrapMode {
  Repeat,
  Clamp
};

class Bitmap : public ITexture {
 public:
  Bitmap(const std::shared_ptr<BlockBasedImage>& image, FilterMode filter, WrapMode wrap)
      : _image(image), _filter(filter), _wrap(wrap) {}

  ~Bitmap() noexcept override {}

  RgbSpectrum Sample(const Vec2& uv) const override {
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
        RgbSpectrum u0v0 = Read(pu, pv);
        RgbSpectrum u1v0 = Read(apu, pv);
        RgbSpectrum u0v1 = Read(pu, apv);
        RgbSpectrum u1v1 = Read(apu, apv);
        auto v = (u0v0 * (1 - du) + u1v0 * du) * (1 - dv) + (u0v1 * (1 - du) + u1v1 * du) * dv;
        return RgbSpectrum(v);
      }
      case FilterMode::Nearest:
      default:
        return Read(pu, pv);
    }
  }

  RgbSpectrum Read(UInt32 x, UInt32 y) const override {
    if (_image->Depth() == 3) {
      return _image->Read3(x, y);
    } else {
      return _image->Read1(x, y);
    }
  }

  UInt32 Width() const override { return _image->Width(); }
  UInt32 Height() const override { return _image->Height(); }

 private:
  Float Wrap(Float v) const {
    switch (_wrap) {
      case WrapMode::Repeat:
        return Clamp(v - std::floor(v), 0, 1);
      case WrapMode::Clamp:
      default:
        return Clamp(v, 0, 1);
    }
  }

  const std::shared_ptr<BlockBasedImage> _image;
  const FilterMode _filter;
  const WrapMode _wrap;
};

}  // namespace rad

namespace rad::factory {
class BitmapFactory : public ITextureFactory {
 public:
  ~BitmapFactory() noexcept override {}
  std::string UniqueId() const override { return "bitmap"; }
  std::unique_ptr<ITexture> Create(const BuildContext* context, const IConfigNode* config) const override {
    std::string assetName = config->GetString("asset_name");
    IImageAsset* image = context->GetImage(assetName);
    std::string filter = config->GetString("filter", "nearest");
    std::string wrapper = config->GetString("wrap", "clamp");
    FilterMode filterMode;
    if (filter == "nearest") {
      filterMode = FilterMode::Nearest;
    } else if (filter == "linear") {
      filterMode = FilterMode::Linear;
    } else {
      filterMode = FilterMode::Nearest;
    }
    WrapMode wrapMode;
    if (wrapper == "clamp") {
      wrapMode = WrapMode::Clamp;
    } else if (wrapper == "repeat") {
      wrapMode = WrapMode::Repeat;
    } else {
      wrapMode = WrapMode::Clamp;
    }
    return std::make_unique<Bitmap>(image->Image(), filterMode, wrapMode);
  }
};
std::unique_ptr<IFactory> CreateBitmapFactory() {
  return std::make_unique<BitmapFactory>();
}
}  // namespace rad::factory
