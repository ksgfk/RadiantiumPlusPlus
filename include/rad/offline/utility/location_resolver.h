#pragma once

#include "../common.h"
#include "../utility/image.h"

#include <string>
#include <filesystem>
#include <fstream>

namespace Rad {

class LocationResolver {
 public:
  LocationResolver(
      const std::filesystem::path& workDir,
      const std::string& projName)
      : _workDir(workDir), _projectName(projName) {}

  inline Unique<std::istream> GetStream(const std::string& location, std::ios::openmode extMode = 0) const {
    auto mode = std::ios::in | extMode;
    std::filesystem::path p(location);
    if (std::filesystem::exists(p)) {
      return std::make_unique<std::ifstream>(std::ifstream(p, mode));
    }
    auto search = _workDir / p;
    if (std::filesystem::exists(search)) {
      return std::make_unique<std::ifstream>(search, mode);
    }
    throw RadFileNotFoundException("cannot find location: {}", location);
  }

  inline void SaveOpenExr(const MatrixX<Spectrum> fb) const {
    std::filesystem::path savePath = _workDir / fmt::format("{}.exr", _projectName);
    std::ofstream stream(savePath, std::ios::out | std::ios::binary);
    Image::SaveOpenExr(stream, fb);
  }

 private:
  std::filesystem::path _workDir;
  std::string _projectName;
};

}  // namespace Rad
