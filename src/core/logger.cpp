#include <cstddef>
#include <radiantium/logger.h>

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace rad {

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

}  // namespace rad
