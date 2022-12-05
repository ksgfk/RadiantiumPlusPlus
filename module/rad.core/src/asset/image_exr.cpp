#include <rad/core/asset.h>

#include <rad/core/logger.h>
#include <rad/core/image_reader.h>

#include <Imath/half.h>

namespace Rad {

/**
 * @brief 读取exr格式文件
 */
class ImageExr final : public ImageAsset {
 public:
  ImageExr(const AssetManager* ctx, const ConfigNode& cfg) : ImageAsset(ctx, cfg) {
    _logger = Logger::GetCategory("ImageEXR");
  }
  ~ImageExr() noexcept override = default;

  Share<MatrixX<Color24f>> ToImageRgb() const override {
    return _rgb;
  }

  Share<MatrixX<Float32>> ToImageMono() const override {
    throw RadNotSupportedException("exr does not support single channel");
  }

  Share<BlockArray2D<Color24f>> ToBlockImageRgb() const override {
    return _blockRgb;
  }

  Share<BlockArray2D<Float32>> ToBlockImageMono() const override {
    throw RadNotSupportedException("exr does not support single channel");
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    auto result = ImageReader::ReadExr(*stream);
    if (!result.IsSuccess) {
      return AssetLoadResult{false, std::move(result.FailReason)};
    }
    if (result.Size.z() != 3) {
      return AssetLoadResult{false, "exr only support 3 channel"};
    }
    _rgb = std::make_shared<MatrixX<Color24f>>(result.Size.x(), result.Size.y());
    size_t width = result.Size.x();
    size_t comSize;
    switch (result.ChannelType) {
      case ImageChannelType::UInt:
        comSize = sizeof(UInt8);
        break;
      case ImageChannelType::Half:
        comSize = sizeof(Imath::half);
        break;
      case ImageChannelType::Float:
        comSize = sizeof(Float32);
        break;
      default:
        throw RadArgumentException("unknown pixel format");
    }
    size_t pixelStride = comSize * 3;
    const std::vector<Byte>& data = result.Data;
    for (UInt32 j = 0; j < (UInt32)result.Size.y(); j++) {
      size_t start = (size_t)j * width;
      for (UInt32 i = 0; i < (UInt32)result.Size.x(); i++) {
        size_t head = (start + i) * pixelStride;
        Color24f rgb;
        switch (result.ChannelType) {
          case ImageChannelType::UInt: {
            UInt8 r = UInt8(data[head]);
            UInt8 g = UInt8(data[head + comSize]);
            UInt8 b = UInt8(data[head + comSize * 2]);
            rgb = Color24f(Float32(r), Float32(g), Float32(b)) / (Float32)std::numeric_limits<UInt8>::max();
            break;
          }
          case ImageChannelType::Half: {
            Imath::half r = *reinterpret_cast<const Imath::half*>(&data[head]);
            Imath::half g = *reinterpret_cast<const Imath::half*>(&data[head + comSize]);
            Imath::half b = *reinterpret_cast<const Imath::half*>(&data[head + comSize * 2]);
            rgb = Color24f(r, g, b);
            break;
          }
          case ImageChannelType::Float: {
            Float32 r = *reinterpret_cast<const Float32*>(&data[head]);
            Float32 g = *reinterpret_cast<const Float32*>(&data[head + comSize]);
            Float32 b = *reinterpret_cast<const Float32*>(&data[head + comSize * 2]);
            rgb = Color24f(r, g, b);
            break;
          }
          default:
            break;
        }
        _rgb->coeffRef(i, j) = rgb;
      }
    }
    return AssetLoadResult{true};
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
  Share<spdlog::logger> _logger;
  Share<MatrixX<Color24f>> _rgb;
  Share<BlockArray2D<Color24f>> _blockRgb;
};

class ImageExrFactory final : public AssetFactory {
 public:
  ImageExrFactory() : AssetFactory("image_exr") {}
  ~ImageExrFactory() noexcept override = default;
  Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<ImageExr>(ctx, cfg);
  }
};

Unique<AssetFactory> _FactoryCreateImageExrFunc_() {
  return std::make_unique<ImageExrFactory>();
}

}  // namespace Rad
