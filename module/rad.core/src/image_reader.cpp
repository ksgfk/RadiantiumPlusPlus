#include <rad/core/image_reader.h>

#include <rad/core/logger.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#if defined(RAD_PLATFORM_WINDOWS)
#define STBI_WINDOWS_UTF8
#endif
#include <stb_image.h>

#include <ImfIO.h>
#include <ImfInputFile.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfPixelType.h>
#include <ImfFrameBuffer.h>
#include <Imath/ImathBox.h>
#include <Imath/half.h>
#include <ImfStringAttribute.h>
#include <ImfOutputFile.h>

namespace Rad {

static stbi_io_callbacks CreateStbIO() {
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
  return cb;
}

ImageReadResult ImageReader::ReadLdrStb(
    std::istream& stream,
    Int32 requireChannel,
    bool isFlipY) {
  static_assert(sizeof(Byte) == sizeof(stbi_uc), "byte size in Rad and stb is inconsistent");
  stbi_io_callbacks cb = CreateStbIO();
  int x, y, channel;
  stbi_set_flip_vertically_on_load_thread(isFlipY);
  auto data = stbi_load_from_callbacks(
      &cb, &stream,
      &x, &y, &channel,
      requireChannel);
  ImageReadResult result{};
  if (data == nullptr) {
    result.IsSuccess = false;
    result.FailReason = stbi_failure_reason();
  } else {
    result.IsSuccess = true;
    result.Size = Eigen::Vector3i(x, y, requireChannel);
    size_t size = (size_t)x * (size_t)y * (size_t)requireChannel;
    result.Data.resize(size, (std::byte)0);
    result.ChannelType = ImageChannelType::UInt;
    std::copy(data, data + size, (stbi_uc*)result.Data.data());
  }
  if (data != nullptr) {
    stbi_image_free(data);
  }
  return result;
}

ImageReadResult ImageReader::ReadHdrStb(
    std::istream& stream,
    Int32 requireChannel,
    bool isFlipY) {
  stbi_io_callbacks cb = CreateStbIO();
  int x, y, channel;
  stbi_set_flip_vertically_on_load_thread(isFlipY);
  auto data = stbi_loadf_from_callbacks(
      &cb, &stream,
      &x, &y, &channel,
      requireChannel);
  ImageReadResult result{};
  if (data == nullptr) {
    result.IsSuccess = false;
    result.FailReason = stbi_failure_reason();
  } else {
    result.IsSuccess = true;
    result.Size = Eigen::Vector3i(x, y, requireChannel);
    size_t size = (size_t)x * (size_t)y * (size_t)requireChannel * sizeof(float);
    result.Data.resize(size, (std::byte)0);
    result.ChannelType = ImageChannelType::Float;
    std::copy(data, data + (x * y * requireChannel), (float*)result.Data.data());
  }
  if (data != nullptr) {
    stbi_image_free(data);
  }
  return result;
}

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

ImageReadResult ImageReader::ReadExr(std::istream& stream) {
  ExrIStream istr(stream);
  ImageReadResult result;
  try {
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
    std::string rgb[] = {{}, {}, {}};       // 通道编码对应名字
    for (auto iter = channels.begin(); iter != channels.end(); iter++) {
      std::string name(iter.name());
      int type = channelType(name);
      if (type == -1) {
        Logger::Get()->warn("exr loader ignore channel: {}", name);
        continue;
      }
      if (isUsed[type]) {
        Logger::Get()->warn("exr loader ignore channel: {}", name);
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
    size_t pixelStride = comSize * 3;        // 一个颜色在exr文件中的大小
    size_t rowStride = pixelStride * width;  // 一行颜色在exr文件中的大小
    std::vector<Byte> data;
    data.resize(rowStride * height, (Byte)0);
    Byte* ptr = data.data() - (dataWindow.min.x + dataWindow.min.y * width) * pixelStride;
    Imf::FrameBuffer framebuffer;  // 将RGB放入fb
    for (size_t i = 0; i < 3; i++) {
      if (!isUsed[i]) {
        continue;
      }
      Imf::Slice s(pixelType, (char*)(ptr + i * comSize), pixelStride, rowStride);
      framebuffer.insert(rgb[i], s);
    }
    file.setFrameBuffer(framebuffer);
    file.readPixels(dataWindow.min.y, dataWindow.max.y);
    result.IsSuccess = true;
    result.Size = Eigen::Vector3i(width, height, 3);
    result.Data = std::move(data);
    switch (pixelType) {
      case Imf::UINT:
        result.ChannelType = ImageChannelType::UInt;
        break;
      case Imf::HALF:
        result.ChannelType = ImageChannelType::Half;
        break;
      case Imf::FLOAT:
        result.ChannelType = ImageChannelType::Float;
        break;
      default:
        throw RadArgumentException("unknown pixel format");
    }
  } catch (const std::exception& e) {
    result.IsSuccess = false;
    result.FailReason = e.what();
  }
  return result;
}

class ExrOStream final : public Imf::OStream {
 public:
  ExrOStream(std::ostream& os) : Imf::OStream(""), _os(&os) {}
  ~ExrOStream() override {}
  void write(const char c[/*n*/], int n) override { _os->write(c, n); }
  uint64_t tellp() override { return _os->tellp(); }
  void seekp(uint64_t pos) override { _os->seekp(pos); }

 private:
  std::ostream* _os;
};

void ImageReader::WriteExr(std::ostream& stream, const MatrixX<Color24f>& img) {
  Imf::Header header((int)img.rows(), (int)img.cols());
  header.insert("comments", Imf::StringAttribute("rad.offline"));
  Imf::ChannelList& channels = header.channels();
  channels.insert("R", Imf::Channel(Imf::FLOAT));
  channels.insert("G", Imf::Channel(Imf::FLOAT));
  channels.insert("B", Imf::Channel(Imf::FLOAT));
  Imf::FrameBuffer frameBuffer;
  size_t compStride = sizeof(Color24f::Scalar);
  size_t pixelStride = sizeof(Color24f);
  size_t rowStride = pixelStride * img.rows();
  char* data = (char*)img.data();
  frameBuffer.insert(
      "R",
      Imf::Slice(Imf::FLOAT, data, pixelStride, rowStride));
  frameBuffer.insert(
      "G",
      Imf::Slice(Imf::FLOAT, data + compStride, pixelStride, rowStride));
  frameBuffer.insert(
      "B",
      Imf::Slice(Imf::FLOAT, data + compStride * 2, pixelStride, rowStride));
  ExrOStream os(stream);
  Imf::OutputFile output(os, header);
  output.setFrameBuffer(frameBuffer);
  output.writePixels((int)img.cols());
}

}  // namespace Rad
