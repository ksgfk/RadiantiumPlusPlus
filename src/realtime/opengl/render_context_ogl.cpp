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

#include <rad/realtime/opengl/render_context_ogl.h>

namespace Rad::OpenGL {

static void DebugCallbackOGL(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    GLchar const* message,
    void const* user_param);

RenderContextOpenGL::RenderContextOpenGL(
    const Window& window,
    const RenderContextOptions& opts) {
  _logger = Logger::GetCategory("OpenGL");
  _ctx = std::make_unique<GLContext>();
  int version = gladLoadGLContext(_ctx.get(), glfwGetProcAddress);
  _logger->info("Load OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#if defined(RAD_DEFINE_DEBUG)
  GLContext* gl = _ctx.get();
  RAD_CHECK_GL(gl, Enable(GL_DEBUG_OUTPUT));
  RAD_CHECK_GL(gl, DebugMessageCallback(DebugCallbackOGL, nullptr));
#endif
  auto driverInfo = RAD_CHECK_GL(gl, GetString(GL_VERSION));
  std::string driver((char*)driverInfo);  //直接强转byte，快进到编码问题（
  auto deviceName = RAD_CHECK_GL(gl, GetString(GL_RENDERER));
  std::string device((char*)deviceName);
  _logger->info("Device: {}", device);
  _logger->info("Driver: {}", driver);
}

void DebugCallbackOGL(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    GLchar const* message,
    void const* user_param) {
  auto typeStr = [](GLenum tp) {
    switch (tp) {
      case GL_DEBUG_TYPE_ERROR:
        return "Error";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated Behavior";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "Undefined Behavior";
      case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability";
      case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance";
      case GL_DEBUG_TYPE_MARKER:
        return "Marker";
      case GL_DEBUG_TYPE_OTHER:
        return "Other";
      default:
        return "Unknown Type";
    }
  }(type);
  if (source == GL_DEBUG_SOURCE_APPLICATION && severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  auto logger = Logger::GetCategory("OpenGL");
  switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      logger->info("id:{}, {}, {}", id, typeStr, message);
    case GL_DEBUG_SEVERITY_LOW:
      logger->warn("id:{}, {}, {}", id, typeStr, message);
    case GL_DEBUG_SEVERITY_MEDIUM:
      logger->error("id:{}, {}, {}", id, typeStr, message);
    case GL_DEBUG_SEVERITY_HIGH:
      logger->critical("id:{}, {}, {}", id, typeStr, message);
    default:
      logger->info("id:{}, {}, {}", id, typeStr, message);
  }
}

}  // namespace Rad::OpenGL
