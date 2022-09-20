#include <rad/offline/utility/image.h>

#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfVersion.h>
#include <OpenEXR/ImfIO.h>

namespace Rad::Image {

class ExrOStream final : public Imf::OStream {
 public:
  ExrOStream(std::ostream& os) : Imf::OStream(""), _os(&os) {}
  ~ExrOStream() override {}
  void write(const char c[/*n*/], int n) override { _os->write(c, n); }
  Imf::Int64 tellp() override { return _os->tellp(); }
  void seekp(Imf::Int64 pos) override { _os->seekp(pos); }

 private:
  std::ostream* _os;
};

void SaveOpenExr(std::ostream& stream, const MatrixX<Color>& img) {
  ExrOStream os(stream);
  Imf::Header header((int)img.rows(), (int)img.cols());
  header.insert("comments", Imf::StringAttribute("rad.offline"));
  Imf::ChannelList& channels = header.channels();
  channels.insert("R", Imf::Channel(Imf::FLOAT));
  channels.insert("G", Imf::Channel(Imf::FLOAT));
  channels.insert("B", Imf::Channel(Imf::FLOAT));
  Imf::FrameBuffer frameBuffer;
  size_t compStride = sizeof(Color::Scalar);
  size_t pixelStride = sizeof(Color);
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
  Imf::OutputFile output(os, header);
  output.setFrameBuffer(frameBuffer);
  output.writePixels((int)img.cols());
}

}  // namespace Rad::Image
