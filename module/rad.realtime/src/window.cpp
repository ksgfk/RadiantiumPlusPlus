#include <rad/realtime/window.h>

#include <rad/core/logger.h>

#include <GLFW/glfw3.h>

namespace Rad {

static GLFWwindow* _window{nullptr};
static Share<spdlog::logger> _logger;
static std::vector<std::function<void(GLFWwindow*, int)>> _windowFocusActions;
static std::vector<std::function<void(GLFWwindow*, int)>> _cursorEnterActions;
static std::vector<std::function<void(GLFWwindow*, Vector2d)>> _cursorPosActions;
static std::vector<std::function<void(GLFWwindow*, int, int, int)>> _mouseButtonActions;
static std::vector<std::function<void(GLFWwindow*, Vector2d)>> _scrollActions;
static std::vector<std::function<void(GLFWwindow*, int, int, int, int)>> _keyEventActions;
static std::vector<std::function<void(GLFWwindow*, unsigned int)>> _charActions;
static std::vector<std::function<void(GLFWmonitor*, int)>> _monitorActions;

void RadInitGlfw() {
  _logger = Logger::GetCategory("glfw");
  glfwSetErrorCallback([](int error, const char* descr) {
    _logger->error("err code:{} , {}", error, descr);
  });
  if (!glfwInit()) {
    throw RadNotSupportedException("glfw初始化失败");
  }
}

void RadShutdownGlfw() {
  _logger = nullptr;
  glfwTerminate();
}

// 万能引用传递参数
// 完美转发把参数类型完整传递到函数
template <typename T, typename... Params>
static void SafeCallDelegates(const std::vector<std::function<T>>& list, Params&&... params) {
  try {
    for (auto&& i : list) {
      i(std::forward<Params>(params)...);
    }
  } catch (const std::exception& e) {
    _logger->error("call delegates exception: {}", e.what());
  }
}

void RadCreateWindowGlfw(const GlfwWindowOptions& opts_) {
  auto opts = opts_;
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
  if (opts.IsMaximize) {
    opts.Size = Vector2i(1280, 720);
  }
  _window = glfwCreateWindow(opts.Size.x(), opts.Size.y(), opts.Title.c_str(), nullptr, nullptr);
  if (opts.IsMaximize) {
    glfwMaximizeWindow(_window);
  }
  if (_window == nullptr) {
    throw RadNotSupportedException("cannot create glfw window");
  }
  glfwSetWindowFocusCallback(_window, [](GLFWwindow* w, int focus) {
    SafeCallDelegates(_windowFocusActions, w, focus);
  });
  glfwSetCursorEnterCallback(_window, [](GLFWwindow* w, int entered) {
    SafeCallDelegates(_cursorEnterActions, w, entered);
  });
  glfwSetCursorPosCallback(_window, [](GLFWwindow* w, double x, double y) {
    Vector2d pos(x, y);
    SafeCallDelegates(_cursorPosActions, w, pos);
  });
  glfwSetMouseButtonCallback(_window, [](GLFWwindow* w, int button, int action, int mods) {
    SafeCallDelegates(_mouseButtonActions, w, button, action, mods);
  });
  glfwSetScrollCallback(_window, [](GLFWwindow* w, double x, double y) {
    Vector2d scroll(x, y);
    SafeCallDelegates(_scrollActions, w, scroll);
  });
  glfwSetKeyCallback(_window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
    SafeCallDelegates(_keyEventActions, w, key, scancode, action, mods);
  });
  glfwSetCharCallback(_window, [](GLFWwindow* w, unsigned int key) {
    SafeCallDelegates(_charActions, w, key);
  });
  glfwSetMonitorCallback([](GLFWmonitor* w, int event) {
    SafeCallDelegates(_monitorActions, w, event);
  });
}

void RadDestroyWindowGlfw() {
  _windowFocusActions.clear();
  _cursorEnterActions.clear();
  _cursorPosActions.clear();
  _mouseButtonActions.clear();
  _scrollActions.clear();
  _keyEventActions.clear();
  _charActions.clear();
  _monitorActions.clear();
  glfwDestroyWindow(_window);
  _window = nullptr;
}

bool RadIsWindowActiveGlfw() {
  return _window != nullptr;
}

GLFWwindow* RadWindowHandlerGlfw() {
  return _window;
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

Vector2i RadGetFrameBufferSizeGlfw() {
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  return Vector2i(width, height);
}

void AddWindowFocusEventGlfw(std::function<void(GLFWwindow*, int)> action) {
  _windowFocusActions.emplace_back(std::move(action));
}
void AddCursorEnterEventGlfw(std::function<void(GLFWwindow*, int)> action) {
  _cursorEnterActions.emplace_back(std::move(action));
}
void AddCursorPosEventGlfw(std::function<void(GLFWwindow*, Vector2d)> action) {
  _cursorPosActions.emplace_back(std::move(action));
}
void AddMouseButtonEventGlfw(std::function<void(GLFWwindow*, int, int, int)> action) {
  _mouseButtonActions.emplace_back(std::move(action));
}
void AddScrollEventGlfw(std::function<void(GLFWwindow*, Vector2d)> action) {
  _scrollActions.emplace_back(std::move(action));
}
void AddKeyEventGlfw(std::function<void(GLFWwindow*, int, int, int, int)> action) {
  _keyEventActions.emplace_back(std::move(action));
}
void AddCharEventGlfw(std::function<void(GLFWwindow*, unsigned int)> action) {
  _charActions.emplace_back(std::move(action));
}
void AddMonitorEventGlfw(std::function<void(GLFWmonitor*, int)> action) {
  _monitorActions.emplace_back(std::move(action));
}

}  // namespace Rad
