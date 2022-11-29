#include <rad/realtime/window.h>

#include <rad/core/logger.h>

#include <utility>

#if defined(RAD_PLATFORM_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif
#include <windows.h>
#endif

namespace Rad {

#if defined(RAD_PLATFORM_WINDOWS)

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

 private:
  Share<spdlog::logger> _logger;
  HWND _hwnd;
  bool _isMinimized{false};
  bool _isMaximized{false};
  bool _isResizing{false};
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

Unique<Window> Window::Create(const WindowOptions& opts) {
#if defined(RAD_PLATFORM_WINDOWS)
  return std::make_unique<Win32Window>(opts);
#else
  throw RadNotImplementedException("不支持的平台, 无法创建窗口");
#endif
}

}  // namespace Rad
