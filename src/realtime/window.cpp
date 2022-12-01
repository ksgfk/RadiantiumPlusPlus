#include <rad/realtime/window.h>

#include <rad/core/logger.h>

#include <utility>
#include <unordered_map>

#if defined(RAD_PLATFORM_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif
#include <windows.h>
#endif

#if defined(RAD_REALTIME_BUILD_OPENGL)
#include <GLFW/glfw3.h>
#endif

namespace Rad {

#if defined(RAD_REALTIME_BUILD_D3D12)
constexpr wchar_t WIN32WINDOWCLASSNAME[] = L"rad_win32_default_class";
static LRESULT CALLBACK Win32WindowMessageCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Win32Window : public Window {
 public:
  Win32Window(const WindowOptions& opts) {
    _logger = Logger::GetCategory("Win32Window");
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Win32WindowMessageCallback;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = nullptr;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = WIN32WINDOWCLASSNAME;
    ATOM cls = RegisterClass(&wc);
    if (!cls) {
      DWORD err = ::GetLastError();
      throw RadNotSupportedException("注册win32 class失败. 错误码: {}", err);
    }
    RECT rect{0, 0, opts.Size.x(), opts.Size.y()};
    ::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    LONG width = rect.right - rect.left;
    LONG height = rect.bottom - rect.top;
    int wstrLen = ::MultiByteToWideChar(CP_UTF8, 0, opts.Title.c_str(), (int)opts.Title.size(), nullptr, 0);
    std::wstring wTitle(wstrLen, '\0');
    ::MultiByteToWideChar(CP_UTF8, 0, opts.Title.c_str(), (int)opts.Title.size(), wTitle.data(), (int)wTitle.length());
    HWND hwnd = ::CreateWindow(
        MAKEINTATOM(cls),
        wTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        0, 0, nullptr, 0);
    if (!hwnd) {
      DWORD err = GetLastError();
      throw RadNotSupportedException("创建win32窗口失败. 错误码: {}", err);
    }
    _hwnd = hwnd;
    _size = opts.Size;
    ::SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  }
  Win32Window(const Win32Window&) = delete;
  Win32Window(Win32Window&& other) noexcept : Window(std::move(other)) {
    _logger = std::move(other._logger);
    _hwnd = other._hwnd;
    other._hwnd = nullptr;
  }
  Win32Window& operator=(const Win32Window&) = delete;
  Win32Window& operator=(Win32Window&& other) noexcept {
    Window::operator=(std::move(other));
    _logger = std::move(other._logger);
    _hwnd = other._hwnd;
    other._hwnd = nullptr;
    return *this;
  }
  ~Win32Window() noexcept override {
    if (_hwnd != nullptr) {
      DestroyWindow(_hwnd);
    }
    _hwnd = nullptr;
  }

  void Show() override {
    if (_hwnd == nullptr) {
      throw RadInvalidOperationException("已经被销毁的窗口");
    }
    ::ShowWindow(_hwnd, SW_SHOW);
    ::UpdateWindow(_hwnd);
  }

  void PollEvent() override {
    MSG msg{0};
    if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        _isClosing = true;
      } else {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

  bool ShouldClose() const override {
    return _isClosing;
  }

  LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
      case WM_CLOSE: {
        _isClosing = true;
        return 0;
      }
      case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        Vector2i newSize(width, height);
        switch (wParam) {
          case SIZE_MINIMIZED: {
            _isMinimized = true;
            _isMaximized = false;
          }
          case SIZE_MAXIMIZED: {
            _isMinimized = false;
            _isMaximized = true;
            _resizeCallbacks.Invoke(*this, newSize);
          }
          case SIZE_RESTORED: {
            if (!_isMinimized && !_isMaximized && !_isResizing) {
              _resizeCallbacks.Invoke(*this, newSize);
            }
            if (_isMinimized) {
              _isMinimized = false;
            }
            if (_isMaximized) {
              _isMaximized = false;
            }
          }
        }
        _size = newSize;
        return 0;
      }
      case WM_ENTERSIZEMOVE: {
        _isResizing = true;
        _startMoveSize = _size;
        return 0;
      }
      case WM_EXITSIZEMOVE: {
        _isResizing = false;
        if (_startMoveSize.cwiseNotEqual(_size).any()) {
          _resizeCallbacks.Invoke(*this, _size);
        }
        return 0;
      }
      default:
        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }
  }

  void AddResizeListener(const std::function<void(Window&, const Vector2i&)>& callback) override {
    _resizeCallbacks.Add(callback);
  }

  void* GetHandler() const override {
    return _hwnd;
  }

  void Destroy() override {
    if (_hwnd != nullptr) {
      DestroyWindow(_hwnd);
    }
    _hwnd = nullptr;
  }

 private:
  Share<spdlog::logger> _logger;
  HWND _hwnd;
  bool _isMinimized{false};
  bool _isMaximized{false};
  bool _isResizing{false};
  bool _isClosing{false};
  Vector2i _startMoveSize;
  MultiDelegate<void(Window&, const Vector2i&)> _resizeCallbacks;
};

static LRESULT CALLBACK Win32WindowMessageCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Win32Window* win = reinterpret_cast<Win32Window*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (win == nullptr) {
    return ::DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return win->ProcessMessage(hwnd, msg, wParam, lParam);
}
#endif

#if defined(RAD_REALTIME_BUILD_OPENGL)
class RadGlfwWindow;
static std::unordered_map<GLFWwindow*, RadGlfwWindow*> RADGLFWINSMAP;
static void OnGlfwResizeCallback(GLFWwindow* win, int x, int y);
static void OnGlfwScrollCallback(GLFWwindow* win, double x, double y);
class RadGlfwWindow : public Window {
 public:
  RadGlfwWindow(const WindowOptions& opts) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(RAD_DEFINE_DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
#endif
    _glfw = glfwCreateWindow(opts.Size.x(), opts.Size.y(), opts.Title.c_str(), nullptr, nullptr);
    if (_glfw == nullptr) {
      throw RadNotSupportedException("无法创建glfw窗口");
    }
    glfwMakeContextCurrent(_glfw);
    glfwSetFramebufferSizeCallback(_glfw, OnGlfwResizeCallback);
    glfwSetScrollCallback(_glfw, OnGlfwScrollCallback);
    auto [_, isIn] = RADGLFWINSMAP.try_emplace(_glfw, this);
    if (!isIn) {
      throw RadNotSupportedException("重复的窗口");
    }
  }
  ~RadGlfwWindow() noexcept override {
    if (_glfw != nullptr) {
      glfwDestroyWindow(_glfw);
      RADGLFWINSMAP.erase(_glfw);
      _glfw = nullptr;
    }
  }

  void Show() override {
    glfwShowWindow(_glfw);
    glfwMakeContextCurrent(_glfw);
  }

  void PollEvent() override {
    glfwPollEvents();
  }

  bool ShouldClose() const override { return glfwWindowShouldClose(_glfw); }

  void AddResizeListener(const std::function<void(Window&, const Vector2i&)>& callback) override {
    _resizeCallbacks.Add(callback);
  }
  void OnResize(const Vector2i& newSize) {
    _resizeCallbacks.Invoke(*this, newSize);
  }
  void AddScrollListener(const std::function<void(Window&, const Vector2f&)>& callback) override {
    _scrollCallbacks.Add(callback);
  }
  void OnScroll(const Vector2f& scroll) {
    _scrollCallbacks.Invoke(*this, scroll);
  }

  void* GetHandler() const override { return _glfw; }

  void Destroy() override {
    if (_glfw != nullptr) {
      glfwDestroyWindow(_glfw);
      RADGLFWINSMAP.erase(_glfw);
      _glfw = nullptr;
    }
  }

 private:
  GLFWwindow* _glfw;
  MultiDelegate<void(Window&, const Vector2i&)> _resizeCallbacks;
  MultiDelegate<void(Window&, const Vector2f&)> _scrollCallbacks;
};
static void OnGlfwResizeCallback(GLFWwindow* win, int x, int y) {
  Vector2i size(x, y);
  RadGlfwWindow* radWin = RADGLFWINSMAP[win];
  radWin->OnResize(size);
}
void OnGlfwScrollCallback(GLFWwindow* win, double x, double y) {
  Vector2f size((float)x, (float)y);
  RadGlfwWindow* radWin = RADGLFWINSMAP[win];
  radWin->OnScroll(size);
}
#endif

void Window::Init() {
#if defined(RAD_REALTIME_BUILD_OPENGL)
  glfwSetErrorCallback([](int error, const char* descr) {
    auto logger = Logger::GetCategory("glfw");
    logger->error("err code:{} , {}", error, descr);
  });
  if (!glfwInit()) {
    throw RadNotSupportedException("glfw初始化失败");
  }
#endif
}

Unique<Window> Window::Create(const WindowOptions& opts) {
  switch (opts.Api) {
    case GraphicsAPI::OpenGL: {
#if defined(RAD_REALTIME_BUILD_OPENGL)
      return std::make_unique<RadGlfwWindow>(opts);
#else
      throw RadNotImplementedException("不支持ogl, 无法创建窗口");
#endif
    }
    case GraphicsAPI::D3D12: {
#if defined(RAD_REALTIME_BUILD_D3D12)
      return std::make_unique<Win32Window>(opts);
#else
      throw RadNotImplementedException("不支持D3D12, 无法创建窗口");
#endif
    }
    default:
      throw RadNotImplementedException("无法创建窗口");
  }
}

void Window::Shutdown() {
  for (auto&& i : RADGLFWINSMAP) {
    glfwDestroyWindow(i.first);
  }
  glfwTerminate();
}

}  // namespace Rad
