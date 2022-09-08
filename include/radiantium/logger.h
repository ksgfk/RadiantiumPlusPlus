#pragma once

#include <spdlog/logger.h>

/**
 * @brief 全局的logger
 * 
 */
namespace rad::logger {

void InitLogger();
void ShutdownLogger();
spdlog::logger* GetLogger();
std::shared_ptr<spdlog::logger> GetCategoryLogger(const std::string& name);

}  // namespace rad
