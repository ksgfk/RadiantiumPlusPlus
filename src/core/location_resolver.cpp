#include <radiantium/location_resolver.h>

#include <fstream>

#include <radiantium/image_utility.h>

using namespace std::filesystem;

namespace rad {

void LocationResolver::SetSearchPath(const path& path) {
  SearchPath = path;
}

std::unique_ptr<std::istream> LocationResolver::GetFileStream(const path& path) const {
  if (exists(path)) {
    return std::make_unique<std::ifstream>(path);
  }
  auto search = SearchPath / path;
  if (exists(search)) {
    return std::make_unique<std::ifstream>(search);
  }
  return nullptr;
}

void DataWriter::SaveExr(const StaticBuffer2D<RgbSpectrum>& tex, const std::string& prefix) const {
  auto file = SavePath / (FileName + (prefix.empty() ? "" : "_" + prefix) + ".exr");
  image::SaveOpenExr(file.string(), tex);
  rad::logger::GetLogger()->info("save exr at: {}", file.string());
}

}  // namespace rad