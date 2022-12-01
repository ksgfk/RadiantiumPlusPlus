#pragma once

#include <rad/core/types.h>

#include <functional>

namespace Rad {

template <class T>
class MultiDelegate {
 public:
  void Add(const std::function<T>& delegate) {
    if (delegate == nullptr) {
      throw RadArgumentException("委托输入不可以是nullptr");
    }
    _delegates.emplace_back(delegate);
  }

  //怎么实现呢, 目前好像还用不到
  // void Remove(const T& delegate) {}

  template <class... Params>
  void Invoke(Params&&... args) const {
    for (const auto& d : _delegates) {
      d(std::forward<Params>(args)...);
    }
  }

 private:
  std::vector<std::function<T>> _delegates;
};

enum class GraphicsAPI {
  OpenGL,
  D3D12
};

struct WindowOptions {
  Vector2i Size;
  std::string Title;
  GraphicsAPI Api;
};

class Window {
 public:
  Window() = default;
  Window(const Window&) = delete;
  Window(Window&&) noexcept = default;
  Window& operator=(const Window&) = delete;
  Window& operator=(Window&&) noexcept = default;
  virtual ~Window() noexcept = default;

  virtual void Show() = 0;
  virtual void PollEvent() = 0;
  virtual bool ShouldClose() const = 0;
  virtual void Destroy() = 0;

  const Vector2i GetSize() const { return _size; }
  virtual void* GetHandler() const = 0;

  virtual void AddResizeListener(const std::function<void(Window&, const Vector2i&)>& callback) = 0;
  virtual void AddScrollListener(const std::function<void(Window&, const Vector2f&)>& callback) = 0;

  static void Init();
  static Unique<Window> Create(const WindowOptions& opts);
  static void Shutdown();

 protected:
  Vector2i _size{};
};

}  // namespace Rad
