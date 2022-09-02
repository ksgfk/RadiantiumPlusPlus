#include <radiantium/image_utility.h>

#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfVersion.h>
#include <OpenEXR/ImfIO.h>

namespace rad {

void SaveOpenExr(const std::string& file, const StaticBuffer<RgbSpectrum>& img) {
  Imf::Header header((int)img.Width(), (int)img.Height());
  header.insert("comments", Imf::StringAttribute("radium"));
  Imf::ChannelList& channels = header.channels();
  channels.insert("R", Imf::Channel(Imf::FLOAT));
  channels.insert("G", Imf::Channel(Imf::FLOAT));
  channels.insert("B", Imf::Channel(Imf::FLOAT));
  Imf::FrameBuffer frameBuffer;
  size_t compStride = sizeof(Float);
  size_t pixelStride = 3 * compStride;
  size_t rowStride = pixelStride * img.Width();
  char* ptr = reinterpret_cast<char*>(img.Data());
  frameBuffer.insert("R", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride));
  ptr += compStride;
  frameBuffer.insert("G", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride));
  ptr += compStride;
  frameBuffer.insert("B", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride));
  Imf::OutputFile outputFile(file.c_str(), header);
  outputFile.setFrameBuffer(frameBuffer);
  outputFile.writePixels((int)img.Height());
}

}  // namespace rad
