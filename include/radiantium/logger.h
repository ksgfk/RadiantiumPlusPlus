#pragma once

#include <spdlog/logger.h>

namespace rad::logger {

void InitLogger();
void ShutdownLogger();
spdlog::logger* GetLogger();
std::shared_ptr<spdlog::logger> GetCategoryLogger(const std::string& name);

}  // namespace rad
