#include <radiantium/asset.h>

#include <radiantium/factory.h>
#include <radiantium/config_node.h>
#include <radiantium/math_ext.h>

#if defined(_MSC_VER)
#define STBI_WINDOWS_UTF8
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace rad::math;

namespace rad {

class DefaultImage : public IImageAsset {
 public:
  DefaultImage(const std::string& location, const std::string& name,
               Int32 channel) : _location(location), _name(name), _reqChannel(channel) {}
  ~DefaultImage() noexcept override {}

  bool IsValid() const override { return _isValid; }
  const std::string& Name() const override { return _name; }
  std::shared_ptr<BlockBasedImage> Image() const override { return _image; }
  void Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    if (!stream->good()) {
      logger::GetLogger()->error("load {} fail, stream is not good", _location);
      _isValid = false;
    }
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
        _reqChannel);
    if (data == nullptr) {
      logger::GetLogger()->error("load {} fail", _location);
      _isValid = false;
    } else {
      std::unique_ptr<Float[]> ft = std::make_unique<Float[]>(x * y * channel);
      for (UInt32 i = 0; i < UInt32(x * y * channel); i++) {
        ft[i] = data[i] / (Float)std::numeric_limits<UInt8>::max();
      }
      _image = std::make_shared<BlockBasedImage>(x, y, channel, ft.get());
    }
    if (data != nullptr) {
      stbi_image_free(data);
    }
  }

 private:
  std::string _location;
  std::string _name;
  UInt32 _reqChannel;
  bool _isValid = false;
  std::shared_ptr<BlockBasedImage> _image;
};

};  // namespace rad

namespace rad::factory {
class DefaultImageFactory : public IAssetFactory {
 public:
  ~DefaultImageFactory() noexcept override {}
  std::string UniqueId() const override { return "image"; }
  std::unique_ptr<IAsset> Create(const BuildContext* context, const IConfigNode* config) const override {
    std::string location = config->GetString("location");
    std::string name = config->GetString("name");
    Int32 channel = config->GetInt32("channel", 3);
    return std::make_unique<DefaultImage>(location, name, channel);
  }
};
std::unique_ptr<IFactory> CreateDefaultImageFactory() {
  return std::make_unique<DefaultImageFactory>();
}

}  // namespace rad::factory
