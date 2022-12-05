#include <rad/core/common.h>
#include <rad/core/logger.h>
#include <rad/realtime/window.h>
#include <rad/realtime/opengl_context.h>

int main(int argc, char** argv) {
  Rad::RadCoreInit();
  Rad::RadInitGlfw();
  try {
    Rad::GlfwWindowOptions opts{};
    opts.Size = {1280, 720};
    opts.Title = "Rad-Offline editor v0.0.1";
    opts.EnableOpenGL = true;
    opts.EnableOpenGLDebugLayer = true;
    Rad::RadCreateWindowGlfw(opts);
    Rad::RadShowWindowGlfw();
    Rad::RadInitContextOpenGL();
    while (!Rad::RadShouldCloseWindowGlfw()) {
      Rad::RadPollEventGlfw();
      glClearColor(0.1f, 0.3f, 0.2f, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      Rad::RadSwapBuffersGlfw();
    }
    Rad::RadShutdownContextOpenGL();
    Rad::RadDestroyWindowGlfw();
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  Rad::RadShutdownGlfw();
  Rad::RadCoreShutdown();
  return 0;
}
