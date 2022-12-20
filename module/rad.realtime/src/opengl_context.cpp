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

#include <GLFW/glfw3.h>

#include <rad/core/logger.h>
#include <rad/realtime/window.h>

namespace Rad {

static Share<spdlog::logger> _logger{nullptr};
static bool _isInitOgl{false};

static void DebugMessage(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    GLchar const* message,
    void const* user_param) {
  auto src = [](GLenum src) {
    switch (src) {
      case GL_DEBUG_SOURCE_API:
        return "API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "Window System";
      case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "Shader Compiler";
      case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "Third Party";
      case GL_DEBUG_SOURCE_APPLICATION:
        return "App";
      case GL_DEBUG_SOURCE_OTHER:
        return "Other";
      default:
        return "Unknown Source";
    }
  };
  if (source == GL_DEBUG_SOURCE_APPLICATION && severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      _logger->debug("[{}] {}", src(source), message);
      break;
    case GL_DEBUG_SEVERITY_LOW:
      _logger->info("[{}] {}", src(source), message);
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      _logger->warn("[{}] {}", src(source), message);
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      _logger->error("[{}] {}", src(source), message);
      break;
  }
}

void RadInitContextOpenGL() {
  if (!RadIsWindowActiveGlfw()) {
    throw RadInvalidOperationException("should create a glfw window before init ogl ctx");
  }
  if (_isInitOgl) {
    throw RadInvalidOperationException("opengl context is active!");
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
  _isInitOgl = true;
}

void RadSetupDebugLayerOpenGL() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(DebugMessage, nullptr);
}

void RadShutdownContextOpenGL() {
  _logger = nullptr;
  _isInitOgl = false;
}

}  // namespace Rad
