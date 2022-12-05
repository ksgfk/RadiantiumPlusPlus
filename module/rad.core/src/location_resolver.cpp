#include <rad/core/location_resolver.h>

#include <fstream>

namespace Rad {

LocationResolver::LocationResolver(
    const std::filesystem::path& workDir)
    : _workDir(workDir) {}

Unique<std::istream> LocationResolver::GetStream(const std::string& location, std::ios::openmode extMode) const {
  auto mode = std::ios::in | extMode;
  std::filesystem::path p(location);
  if (std::filesystem::exists(p)) {
    return std::make_unique<std::ifstream>(p, mode);
  }
  auto search = _workDir / p;
  if (std::filesystem::exists(search)) {
    return std::make_unique<std::ifstream>(search, mode);
  }
  throw RadFileNotFoundException("cannot find location: {}", location);
}

Unique<std::ostream> LocationResolver::WriteStream(const std::string& location, std::ios::openmode extMode) const {
  auto mode = std::ios::out | extMode;
  std::filesystem::path p(location);
  auto search = _workDir / p;
  return std::make_unique<std::ofstream>(search, mode);
}

std::string LocationResolver::GetSaveName(const std::string& ext) const {
  return fmt::format("{}.{}", _saveName, ext);
}

}  // namespace Rad
