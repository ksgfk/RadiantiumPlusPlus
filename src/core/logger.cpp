#include <cstddef>
#include <radiantium/logger.h>

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace rad::logger {

static std::shared_ptr<spdlog::logger> _globalLogger;

void InitLogger() {
  if (_globalLogger != nullptr) {
    return;
  }
  _globalLogger = spdlog::create<spdlog::sinks::stdout_color_sink_mt>("mian");
}

void ShutdownLogger() {
  if (_globalLogger == nullptr) {
    return;
  }
  _globalLogger->flush();
  spdlog::shutdown();
  _globalLogger = nullptr;
}

spdlog::logger* GetLogger() {
  return _globalLogger.get();
}

std::shared_ptr<spdlog::logger> GetCategoryLogger(const std::string& name) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get(name);
  if (logger == nullptr) {
    logger = _globalLogger->clone(name);
  }
  return logger;
}

}  // namespace rad::logger
