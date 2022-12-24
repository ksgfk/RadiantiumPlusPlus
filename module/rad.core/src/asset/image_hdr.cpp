#include <rad/core/asset.h>

#include <rad/core/image_reader.h>

namespace Rad {

/**
 * @brief 使用stb_image读取图片
 */
class ImageHdr final : public ImageAsset {
 public:
  ImageHdr(const AssetManager* ctx, const ConfigNode& cfg) : ImageAsset(ctx, cfg) {
    _isFlipY = cfg.ReadOrDefault("is_flip_y", false);
  }
  ~ImageHdr() noexcept override = default;

  Share<MatrixX<Color24f>> ToImageRgb() const override {
    return _rgb;
  }

  Share<MatrixX<Float32>> ToImageMono() const override {
    throw RadInvalidOperationException("loaded image {} is rgb", _name);
  }

  Share<BlockArray2D<Color24f>> ToBlockImageRgb() const override {
    return _blockRgb;
  }

  Share<BlockArray2D<Float32>> ToBlockImageMono() const override {
    throw RadInvalidOperationException("loaded image {} is rgb", _name);
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    auto result = ImageReader::ReadHdrStb(*stream, 3, _isFlipY);
    if (!result.IsSuccess) {
      return AssetLoadResult{false, std::move(result.FailReason)};
    }
    if (result.ChannelType != ImageChannelType::Float) {
      return AssetLoadResult{false, "unknown data type"};
    }
    AssetLoadResult r{};
    _rgb = std::make_shared<MatrixX<Color24f>>(result.Size.x(), result.Size.y());
    float* ptr = (float*)result.Data.data();
    for (UInt32 j = 0; j < (UInt32)result.Size.y(); j++) {
      for (UInt32 i = 0; i < (UInt32)result.Size.x(); i++) {
        float r = ptr[0];
        float g = ptr[1];
        float b = ptr[2];
        ptr += 3;
        Color24f color(r, g, b);
        _rgb->coeffRef(i, j) = color;
      }
    }
    r.IsSuccess = true;
    return r;
  }

  void GenerateBlockBasedImage() override {
    _blockRgb = std::make_shared<BlockArray2D<Color24f>>(_rgb->rows(), _rgb->cols());
    for (UInt32 j = 0; j < _rgb->cols(); j++) {
      for (UInt32 i = 0; i < _rgb->rows(); i++) {
        (*_blockRgb)(i, j) = _rgb->coeff(i, j);
      }
    }
  }

 private:
  bool _isFlipY;
  Share<MatrixX<Color24f>> _rgb;
  Share<BlockArray2D<Color24f>> _blockRgb;
};

class ImageHdrFactory final : public AssetFactory {
 public:
  ImageHdrFactory() : AssetFactory("image_hdr") {}
  ~ImageHdrFactory() noexcept override = default;
  Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<ImageHdr>(ctx, cfg);
  }
};

Unique<AssetFactory> _FactoryCreateImageHdrFunc_() {
  return std::make_unique<ImageHdrFactory>();
}

}  // namespace Rad
