#include "editor_application.h"

#include <rad/realtime/window.h>
#include <rad/realtime/input.h>

#include <rad/realtime/opengl/render_context_ogl.h>

namespace Rad::Editor {

class EditorApplicationImpl {
 public:
  EditorApplicationImpl(int argc, char** argv) {
    Rad::Window::Init();
    WindowOptions wo{Vector2i(1280, 720), "Radiantium Editor v0.0.1"};
    _window = Window::Create(wo);
    _input = std::make_unique<Input>();
    _window->AddScrollListener([&](auto& w, const auto& s) { _input->OnScroll(s); });
    RenderContextOptions rco{};
    rco.EnableDebug = true;
    _renderCtx = std::make_unique<OpenGL::RenderContextOpenGL>(*_window, rco);
  }
  ~EditorApplicationImpl() {
    _input = nullptr;
    _renderCtx = nullptr;
    _window->Destroy();
    Rad::Window::Shutdown();
  }

  void Run() {
    while (!_window->ShouldClose()) {
      _window->PollEvent();
      _input->Update(*_window);
      _renderCtx->SwapBuffers();
      _renderCtx->GetGL()->ClearColor(0, 0.2f, 0.3f, 0.1f);
      _renderCtx->GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  }

 private:
  Unique<Window> _window;
  Unique<Input> _input;
  Unique<OpenGL::RenderContextOpenGL> _renderCtx;
};

EditorApplication::EditorApplication(int argc, char** argv) {
  _impl = std::make_shared<EditorApplicationImpl>(argc, argv);
}

void EditorApplication::Run() {
  _impl->Run();
}

}  // namespace Rad::Editor
