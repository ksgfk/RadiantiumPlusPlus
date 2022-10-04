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
    _channel = cfg.ReadOrDefault("channel", 3);
    _isFlipY = cfg.ReadOrDefault("is_flip_y", true);
    _isToLinear = cfg.ReadOrDefault("is_to_linear", _channel == 3);
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
      std::istream* stream = reinterpret_cast<std::istream*>(user);
      stream->read(data, static_cast<std::streamsize>(size));
      int count = static_cast<int>(stream->gcount());
      return count;
    };
    cb.skip = [](void* user, int n) {
      std::istream* stream = reinterpret_cast<std::istream*>(user);
      stream->ignore(static_cast<std::streamsize>(n));
    };
    cb.eof = [](void* user) -> int {
      std::istream* stream = reinterpret_cast<std::istream*>(user);
      return stream->eof() || stream->bad() || stream->fail();
    };
    int x, y, channel;
    stbi_set_flip_vertically_on_load(_isFlipY);
    auto data = stbi_load_from_callbacks(
        &cb,
        stream.get(), &x, &y, &channel,
        _channel);
    AssetLoadResult result;
    if (data == nullptr) {
      result.IsSuccess = false;
      result.FailResult = stbi_failure_reason();
    } else {
      if (_channel == 3) {
        _rgb = std::make_shared<BlockBasedImage<Color>>(x, y);
        UInt8* ptr = data;
        for (UInt32 j = 0; j < UInt32(y); j++) {
          for (UInt32 i = 0; i < UInt32(x); i++) {
            UInt8 r = ptr[0];
            UInt8 g = ptr[1];
            UInt8 b = ptr[2];
            ptr += 3;
            Float32 fr = Float32(r) / std::numeric_limits<UInt8>::max();
            Float32 fg = Float32(g) / std::numeric_limits<UInt8>::max();
            Float32 fb = Float32(b) / std::numeric_limits<UInt8>::max();
            Color color(fr, fg, fb);
            if (_isToLinear) {
              color = Color::ToLinear(color);
            }
            _rgb->Write(i, j, color);
          }
        }
        result.IsSuccess = true;
      } else if (_channel == 1) {
        _gray = std::make_shared<BlockBasedImage<Float32>>(x, y);
        UInt8* ptr = data;
        for (UInt32 j = 0; j < UInt32(y); j++) {
          for (UInt32 i = 0; i < UInt32(x); i++) {
            UInt8 gray = *ptr;
            ptr++;
            Float32 color = Float32(gray) / std::numeric_limits<UInt8>::max();
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
  Int32 _channel;
  bool _isFlipY;
  bool _isToLinear;
  Share<BlockBasedImage<Color>> _rgb;
  Share<BlockBasedImage<Float32>> _gray;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(ImageDefault, "image_default");
