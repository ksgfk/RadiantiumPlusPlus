#pragma once

#include <filesystem>
#include <istream>
#include <memory>

#include "static_buffer.h"
#include "spectrum.h"

namespace rad {

using path = std::filesystem::path;

class LocationResolver {
 public:
  void SetSearchPath(const path& path);
  std::unique_ptr<std::istream> GetStream(const path& path, std::ios::openmode extMode = 0) const;

  path SearchPath;
};

class DataWriter {
 public:
  void SaveExr(const StaticBuffer2D<RgbSpectrum>& tex, const std::string& prefix = "") const;

  path SavePath;
  std::string FileName;
};

}  // namespace rad
