#include <rad/offline/asset/asset.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

#include <ImfIO.h>
#include <ImfInputFile.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfPixelType.h>
#include <ImfFrameBuffer.h>
#include <Imath/ImathBox.h>
#include <Imath/half.h>

namespace Rad {

class ExrIStream final : public Imf::IStream {
 public:
  ExrIStream(std::istream& os) : Imf::IStream(""), _is(&os) {}
  ~ExrIStream() override {}
  bool read(char c[/*n*/], int n) override {
    _is->read(c, n);
    return !(_is->eof() || _is->bad() || _is->fail());
  }
  uint64_t tellg() override { return _is->tellg(); }
  void seekg(uint64_t pos) override { _is->seekg(pos); }

 private:
  std::istream* _is;
};

/**
 * @brief 只加载RGB颜色, 格式是扫描线的exr文件
 *
 */
class ImageExr final : public ImageAsset {
 public:
  ImageExr(BuildContext* ctx, const ConfigNode& cfg) : ImageAsset(ctx, cfg) {
    _logger = Logger::GetCategory("ImageEXR");
  }
  ~ImageExr() noexcept override = default;

  Share<BlockBasedImage<Color>> ToImageRgb() const override {
    return _rgb;
  }

  Share<BlockBasedImage<Float32>> ToImageGray() const override {
    throw RadInvalidOperationException("exr does not support single channel");
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    try {
      auto stream = resolver.GetStream(_location, std::ios::binary);
      ExrIStream istr(*stream);
      Imf::InputFile file(istr);
      const Imf::Header& header = file.header();
      const Imf::ChannelList& channels = header.channels();
      /*
       * 将channel名字转化为通道编码, 我们只寻找RGB通道, 如果不满则填充0
       * 通道编码:
       *  -1: 未知的通道
       *  0: R通道
       *  1: G通道
       *  2: B通道
       */
      auto channelType = [](std::string name) -> Int32 {
        auto it = name.rfind(".");
        if (it != std::string::npos) {
          name = name.substr(it + 1);
        }
        for (size_t i = 0; i < name.length(); ++i) {
          name[i] = (char)std::tolower(name[i]);
        }
        if (name == "r") return 0;
        if (name == "g") return 1;
        if (name == "b") return 2;
        return -1;
      };
      bool isUsed[] = {false, false, false};  // exr文件是否拥有通道
      std::string rgb[] = {{}, {}, {}};       //通道编码对应名字
      for (auto iter = channels.begin(); iter != channels.end(); iter++) {
        std::string name(iter.name());
        int type = channelType(name);
        if (type == -1) {
          _logger->warn("ignore channel: {}", name);
          continue;
        }
        if (isUsed[type]) {
          _logger->warn("ignore channel: {}", name);
          continue;
        }
        isUsed[type] = true;
        rgb[type] = name;
      }
      Imf::PixelType pixelType = channels.begin().channel().type;
      Imath::Box2i dataWindow = file.header().dataWindow();
      Int32 width = dataWindow.max.x - dataWindow.min.x + 1;
      Int32 height = dataWindow.max.y - dataWindow.min.y + 1;
      size_t comSize;  // RGB每个分量在exr文件中的大小
      switch (pixelType) {
        case Imf::UINT:
          comSize = sizeof(UInt8);
          break;
        case Imf::HALF:
          comSize = sizeof(Imath::half);
          break;
        case Imf::FLOAT:
          comSize = sizeof(Float32);
          break;
        default:
          throw RadArgumentException("unknown pixel format");
      }
      size_t pixelStride = comSize * 3;        //一个颜色在exr文件中的大小
      size_t rowStride = pixelStride * width;  //一行颜色在exr文件中的大小
      std::vector<UInt8> data;
      data.resize(rowStride * height, 0);
      UInt8* ptr = data.data() - (dataWindow.min.x + dataWindow.min.y * width) * pixelStride;
      Imf::FrameBuffer framebuffer;  //将RGB放入fb
      for (size_t i = 0; i < 3; i++) {
        if (!isUsed[i]) {
          continue;
        }
        Imf::Slice s(pixelType, (char*)(ptr + i * comSize), pixelStride, rowStride);
        framebuffer.insert(rgb[i], s);
      }
      file.setFrameBuffer(framebuffer);
      file.readPixels(dataWindow.min.y, dataWindow.max.y);
      _rgb = std::make_shared<BlockBasedImage<Color>>(width, height);
      //将读入的数据转化为我们需要的数据
      for (UInt32 j = 0; j < UInt32(height); j++) {
        size_t start = j * width;
        for (UInt32 i = 0; i < UInt32(width); i++) {
          size_t head = (start + i) * pixelStride;
          Color rgb;
          switch (pixelType) {
            case Imf::UINT: {
              UInt8 r = data[head];
              UInt8 g = data[head + comSize];
              UInt8 b = data[head + comSize * 2];
              Float32 fr = Float32(r) / std::numeric_limits<UInt8>::max();
              Float32 fg = Float32(g) / std::numeric_limits<UInt8>::max();
              Float32 fb = Float32(b) / std::numeric_limits<UInt8>::max();
              rgb = Color(fr, fg, fb);
              break;
            }
            case Imf::HALF: {
              Imath::half r = *reinterpret_cast<Imath::half*>(&data[head]);
              Imath::half g = *reinterpret_cast<Imath::half*>(&data[head + comSize]);
              Imath::half b = *reinterpret_cast<Imath::half*>(&data[head + comSize * 2]);
              rgb = Color(r, g, b);
              break;
            }
            case Imf::FLOAT: {
              Float32 r = *reinterpret_cast<Float32*>(&data[head]);
              Float32 g = *reinterpret_cast<Float32*>(&data[head + comSize]);
              Float32 b = *reinterpret_cast<Float32*>(&data[head + comSize * 2]);
              rgb = Color(r, g, b);
              break;
            }
            default: {
              break;
            }
          }
          _rgb->Write(i, j, rgb);
        }
      }
    } catch (const std::exception& e) {
      return AssetLoadResult{false, e.what()};
    }
    return AssetLoadResult{true};
  }

 private:
  Share<spdlog::logger> _logger;
  Share<BlockBasedImage<Color>> _rgb;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(ImageExr, "image_exr");
