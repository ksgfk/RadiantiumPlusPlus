#include <rad/core/common.h>
#include <rad/core/logger.h>

#include <rad/offline/editor/application.h>

int main(int argc, char** argv) {
  Rad::RadCoreInit();
  try {
    auto app = std::make_unique<Rad::Application>(argc, argv);
    app->Run();
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  Rad::RadCoreShutdown();
  return 0;
}
