#if defined(_MSC_VER)
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#ifdef __cplusplus
}
#endif
#endif

#include <rad/realtime/opengl_context.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/realtime/window.h>

namespace Rad {

static Share<spdlog::logger> _logger{nullptr};

void RadInitContextOpenGL() {
  if (!RadIsWindowActiveGlfw()) {
    throw RadInvalidOperationException("should create a glfw window before init ogl ctx");
  }
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    throw RadInvalidOperationException("cannot init opengl context");
  }
  _logger = Logger::GetCategory("OpenGL");
  _logger->info("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  auto driverInfo = glGetString(GL_VERSION);
  std::string driver((char*)driverInfo);
  auto deviceName = glGetString(GL_RENDERER);
  std::string device((char*)deviceName);
  _logger->info("Device: {}", device);
  _logger->info("Driver: {}", driver);
}

void RadShutdownContextOpenGL() {
  _logger = nullptr;
}

}  // namespace Rad
