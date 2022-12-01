#include <rad/core/logger.h>
#include <rad/offline/api.h>

#include "editor_application.h"

int main(int argc, char** argv) {
  RadInit();
  try {
    Rad::Editor::EditorApplication editor(argc, argv);
    editor.Run();
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  RadShutdown();
  return 0;
}
