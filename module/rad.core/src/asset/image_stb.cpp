#include <rad/core/asset.h>

#include <rad/core/image_reader.h>

namespace Rad {

/**
 * @brief 使用stb_image读取图片
 */
class ImageStb final : public ImageAsset {
 public:
  ImageStb(const AssetManager* ctx, const ConfigNode& cfg) : ImageAsset(ctx, cfg) {
    _channel = cfg.ReadOrDefault("channel", 3);
    _isFlipY = cfg.ReadOrDefault("is_flip_y", true);
    _isToLinear = cfg.ReadOrDefault("is_to_linear", _channel == 3);
  }
  ~ImageStb() noexcept override = default;

  Share<MatrixX<Color24f>> ToImageRgb() const override {
    if (_channel == 3) {
      return _rgb;
    }
    throw RadInvalidOperationException("loaded image {} is mono", _name);
  }

  Share<MatrixX<Float32>> ToImageMono() const override {
    if (_channel == 1) {
      return _mono;
    }
    throw RadInvalidOperationException("loaded image {} is rgb", _name);
  }

  Share<BlockArray2D<Color24f>> ToBlockImageRgb() const override {
    if (_channel == 3) {
      return _blockRgb;
    }
    throw RadInvalidOperationException("loaded image {} is mono", _name);
  }

  Share<BlockArray2D<Float32>> ToBlockImageMono() const override {
    if (_channel == 1) {
      return _blockMono;
    }
    throw RadInvalidOperationException("loaded image {} is rgb", _name);
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    auto result = ImageReader::ReadLdrStb(*stream, _channel, _isFlipY);
    if (!result.IsSuccess) {
      return AssetLoadResult{false, std::move(result.FailReason)};
    }
    if (result.ChannelType != ImageChannelType::UInt) {
      return AssetLoadResult{false, "unknown data type"};
    }
    AssetLoadResult r{};
    if (result.Size.z() == 3) {
      _rgb = std::make_shared<MatrixX<Color24f>>(result.Size.x(), result.Size.y());
      Byte* ptr = result.Data.data();
      for (UInt32 j = 0; j < (UInt32)result.Size.y(); j++) {
        for (UInt32 i = 0; i < (UInt32)result.Size.x(); i++) {
          Byte r = ptr[0];
          Byte g = ptr[1];
          Byte b = ptr[2];
          ptr += 3;
          Float32 fr = Float32(r);
          Float32 fg = Float32(g);
          Float32 fb = Float32(b);
          Color24f color(fr, fg, fb);
          color /= std::numeric_limits<UInt8>::max();
          _rgb->coeffRef(i, j) = color;
        }
      }
      if (_isToLinear) {
        for (UInt32 j = 0; j < (UInt32)result.Size.y(); j++) {
          for (UInt32 i = 0; i < (UInt32)result.Size.x(); i++) {
            Color24f gamma = _rgb->coeff(i, j);
            Color24f linear = Color24f::ToLinear(gamma);
            _rgb->coeffRef(i, j) = linear;
          }
        }
      }
      r.IsSuccess = true;
    } else if (result.Size.z() == 1) {
      _mono = std::make_shared<MatrixX<Float32>>(result.Size.x(), result.Size.y());
      Byte* ptr = result.Data.data();
      for (UInt32 j = 0; j < (UInt32)result.Size.y(); j++) {
        for (UInt32 i = 0; i < (UInt32)result.Size.x(); i++) {
          Byte r = ptr[0];
          ptr++;
          Float32 fr = Float32(r) / std::numeric_limits<UInt8>::max();
          _mono->coeffRef(i, j) = fr;
        }
      }
      r.IsSuccess = true;
    } else {
      r.IsSuccess = false;
      r.FailReason = fmt::format("channel must be 1 or 3. load image {} channel is {}", _location, result.Size.z());
    }
    return r;
  }

  void GenerateBlockBasedImage() override {
    if (_rgb != nullptr) {
      _blockRgb = std::make_shared<BlockArray2D<Color24f>>(_rgb->rows(), _rgb->cols());
      for (UInt32 j = 0; j < _rgb->cols(); j++) {
        for (UInt32 i = 0; i < _rgb->rows(); i++) {
          (*_blockRgb)(i, j) = _rgb->coeff(i, j);
        }
      }
    }
    if (_mono != nullptr) {
      _blockMono = std::make_shared<BlockArray2D<Float32>>(_mono->rows(), _mono->cols());
      for (UInt32 j = 0; j < _mono->cols(); j++) {
        for (UInt32 i = 0; i < _mono->rows(); i++) {
          (*_blockMono)(i, j) = _mono->coeff(i, j);
        }
      }
    }
  }

 private:
  Int32 _channel;
  bool _isFlipY;
  bool _isToLinear;
  Share<MatrixX<Color24f>> _rgb;
  Share<MatrixX<Float32>> _mono;
  Share<BlockArray2D<Color24f>> _blockRgb;
  Share<BlockArray2D<Float32>> _blockMono;
};

class ImageStbFactory final : public AssetFactory {
 public:
  ImageStbFactory() : AssetFactory("image_default") {}
  ~ImageStbFactory() noexcept override = default;
  Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<ImageStb>(ctx, cfg);
  }
};

Unique<AssetFactory> _FactoryCreateImageStbFunc_() {
  return std::make_unique<ImageStbFactory>();
}

}  // namespace Rad
