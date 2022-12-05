#pragma once

#include "types.h"

#include <string>
#include <filesystem>
#include <istream>
#include <ostream>

namespace Rad {

/**
 * @brief 工具类，用于拼接文件路径，获取数据流
 */
class RAD_EXPORT_API LocationResolver {
 public:
  LocationResolver(const std::filesystem::path& workDir);

  void SetSaveName(const std::string& name) { _saveName = name; }

  const std::filesystem::path& GetWorkDirectory() const { return _workDir; }
  Unique<std::istream> GetStream(const std::string& location, std::ios::openmode extMode = 0) const;
  Unique<std::ostream> WriteStream(const std::string& location, std::ios::openmode extMode = 0) const;
  std::string GetSaveName(const std::string& ext) const;

 private:
  std::filesystem::path _workDir;
  std::string _saveName;
};

}  // namespace Rad
