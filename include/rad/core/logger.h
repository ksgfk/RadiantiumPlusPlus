#pragma once

#include "types.h"

namespace Rad {

class RAD_EXPORT_API Logger {
 public:
  static void Init();
  static void Shutdown();
  static spdlog::logger* Get();
  static Share<spdlog::logger> GetCategory(const std::string& name);
  static void SetDefaultLevel(spdlog::level::level_enum level);
};

}  // namespace Rad::Logger
