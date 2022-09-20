#include <rad/offline/asset/asset.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#if defined(RAD_PLATFORM_WINDOWS)
#define STBI_WINDOWS_UTF8
#endif
#include <stb_image.h>

namespace Rad {

class ImageDefault final : public ImageAsset {
 public:
  ImageDefault(BuildContext* ctx, const ConfigNode& cfg) : ImageAsset(ctx, cfg) {
    _location = cfg.Read<std::string>("location");
    _channel = cfg.ReadOrDefault("channel", 3);
  }
  ~ImageDefault() noexcept override = default;

  Share<BlockBasedImage<Color>> ToImageRgb() const override {
    if (_channel == 3) {
      return _rgb;
    }
    auto rgb = std::make_shared<BlockBasedImage<Color>>(_gray->Width(), _gray->Height());
    for (UInt32 j = 0; j < rgb->Height(); j++) {
      for (UInt32 i = 0; i < rgb->Width(); i++) {
        rgb->Write(i, j, Color(_gray->Read(i, j)));
      }
    }
    return rgb;
  }

  Share<BlockBasedImage<Float32>> ToImageGray() const override {
    if (_channel == 1) {
      return _gray;
    }
    auto gray = std::make_shared<BlockBasedImage<Float32>>(_rgb->Width(), _rgb->Height());
    for (UInt32 j = 0; j < gray->Height(); j++) {
      for (UInt32 i = 0; i < gray->Width(); i++) {
        gray->Write(i, j, _rgb->Read(i, j).R());
      }
    }
    return gray;
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    stbi_io_callbacks cb;
    cb.read = [](void* user, char* data, int size) -> int {
      std::istream* stream = (std::istream*)user;
      stream->read(data, size);
      return (int)stream->gcount();
    };
    cb.skip = [](void* user, int n) {
      std::istream* stream = (std::istream*)user;
      if (n >= 0) {
        for (int i = 0; i < n; i++) {
          if (stream->eof()) {
            break;
          }
          stream->get();
        }
      } else {
        for (int i = 0; i < -n; i++) {
          stream->unget();
        }
      }
    };
    cb.eof = [](void* user) -> int {
      std::istream* stream = (std::istream*)user;
      return stream->eof() ? 1 : 0;
    };
    int x, y, channel;
    auto data = stbi_load_from_callbacks(
        &cb,
        stream.get(), &x, &y, &channel,
        _channel);
    AssetLoadResult result;
    if (data == nullptr) {
      result.IsSuccess = false;
      result.FailResult = stbi_failure_reason();
    } else {
      if (channel == 3) {
        _rgb = std::make_shared<BlockBasedImage<Color>>(x, y);
        for (UInt32 j = 0; j < UInt32(y); j++) {
          for (UInt32 i = 0; i < UInt32(x); i++) {
            UInt32 p = (j * x + i) * channel;
            UInt8 r = data[p + 0];
            UInt8 g = data[p + 1];
            UInt8 b = data[p + 2];
            auto color = Eigen::Vector<UInt8, 3>(r, g, b).cast<Float32>() /
                         (Float32)std::numeric_limits<UInt8>::max();
            _rgb->Write(i, j, Color(color));
          }
        }
        result.IsSuccess = true;
      } else if (channel == 1) {
        _gray = std::make_shared<BlockBasedImage<Float32>>(x, y);
        for (UInt32 j = 0; j < UInt32(y); j++) {
          for (UInt32 i = 0; i < UInt32(x); i++) {
            UInt8 gray = data[j * x + i];
            Float32 color = gray / (Float32)std::numeric_limits<UInt8>::max();
            _gray->Write(i, j, color);
          }
        }
        result.IsSuccess = true;
      } else {
        result.IsSuccess = false;
        result.FailResult = "channel must be 1 or 3";
      }
    }
    if (data != nullptr) {
      stbi_image_free(data);
    }
    return result;
  }

 private:
  std::string _location;
  Int32 _channel;
  Share<BlockBasedImage<Color>> _rgb;
  Share<BlockBasedImage<Float32>> _gray;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(ImageDefault, "image_default");
