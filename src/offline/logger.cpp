#include <rad/offline/logger.h>

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Rad {

static std::shared_ptr<spdlog::logger> _globalLogger;

void Logger::Init() {
  if (_globalLogger != nullptr) {
    return;
  }
  _globalLogger = spdlog::create<spdlog::sinks::stdout_color_sink_mt>("mian");
#if defined(RAD_DEFINE_DEBUG)
  _globalLogger->set_level(spdlog::level::debug);
#endif
}

void Logger::Shutdown() {
  if (_globalLogger == nullptr) {
    return;
  }
  _globalLogger->flush();
  spdlog::shutdown();
  _globalLogger = nullptr;
}

spdlog::logger* Logger::Get() {
  return _globalLogger.get();
}

Share<spdlog::logger> Logger::GetCategory(const std::string& name) {
  Share<spdlog::logger> logger = spdlog::get(name);
  if (logger == nullptr) {
    logger = _globalLogger->clone(name);
  }
  return logger;
}

}  // namespace Rad
