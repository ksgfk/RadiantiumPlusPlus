#include <rad/core/logger.h>

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Rad {

void Logger::Init() {
  auto global = spdlog::create<spdlog::sinks::stdout_color_sink_mt>("mian");
#if defined(RAD_DEFINE_DEBUG)
  global->set_level(spdlog::level::debug);
#endif
  spdlog::set_default_logger(std::move(global));
}

void Logger::Shutdown() {
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) { logger->flush(); });
  spdlog::shutdown();
}

spdlog::logger* Logger::Get() {
  return spdlog::default_logger_raw();
}

Share<spdlog::logger> Logger::GetCategory(const std::string& name) {
  Share<spdlog::logger> logger = spdlog::get(name);
  if (logger == nullptr) {
    logger = spdlog::default_logger_raw()->clone(name);
  }
  return logger;
}

void Logger::SetDefaultLevel(spdlog::level::level_enum level) {
  spdlog::default_logger_raw()->set_level(level);
}

}  // namespace Rad
