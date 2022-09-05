#pragma once

#include <spdlog/logger.h>

namespace rad::logger {

void InitLogger();
void ShutdownLogger();
spdlog::logger* GetLogger();

}  // namespace rad
