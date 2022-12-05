#include <rad/realtime/window.h>

#include <rad/core/logger.h>

#include <GLFW/glfw3.h>

namespace Rad {

static GLFWwindow* _window{nullptr};

void RadInitGlfw() {
  glfwSetErrorCallback([](int error, const char* descr) {
    auto logger = Logger::GetCategory("glfw");
    logger->error("err code:{} , {}", error, descr);
  });
  if (!glfwInit()) {
    throw RadNotSupportedException("glfw初始化失败");
  }
}

void RadShutdownGlfw() {
  glfwTerminate();
}

void RadCreateWindowGlfw(const GlfwWindowOptions& opts) {
  if (_window != nullptr) {
    throw RadNotSupportedException("rad do not support multi window");
  }
  if (opts.EnableOpenGL) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (opts.EnableOpenGLDebugLayer) {
      glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    } else {
      glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
    }
  } else {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  }
  _window = glfwCreateWindow(opts.Size.x(), opts.Size.y(), opts.Title.c_str(), nullptr, nullptr);
  if (_window == nullptr) {
    throw RadNotSupportedException("cannot create glfw window");
  }
}

void RadDestroyWindowGlfw() {
  glfwDestroyWindow(_window);
  _window = nullptr;
}

bool RadIsWindowActiveGlfw() {
  return _window != nullptr;
}

void RadShowWindowGlfw() {
  glfwShowWindow(_window);
  glfwMakeContextCurrent(_window);
}

bool RadShouldCloseWindowGlfw() {
  return glfwWindowShouldClose(_window);
}

void RadPollEventGlfw() {
  return glfwPollEvents();
}

void RadSwapBuffersGlfw() {
  glfwSwapBuffers(_window);
}

}  // namespace Rad
