#pragma once

#include "types.h"

namespace Rad {

class Logger {
 public:
  static void Init();
  static void Shutdown();
  static spdlog::logger* Get();
  static Share<spdlog::logger> GetCategory(const std::string& name);
};

}  // namespace Rad::Logger
