#pragma once

#include <spdlog/logger.h>

namespace rad {

void InitLogger();
void ShutdownLogger();
spdlog::logger* GetLogger();

}  // namespace rad
