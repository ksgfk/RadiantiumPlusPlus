#include <rad/realtime/window.h>
#include <rad/realtime/input.h>
#include <rad/core/logger.h>

#include <rad/realtime/opengl/render_context_ogl.h>

using namespace Rad;

int main() {
  Window::Init();
  WindowOptions opts{Vector2i(1280, 720), "芜湖，起飞"};
  Unique<Window> w = Window::Create(opts);
  Input input{};
  w->AddScrollListener([&](auto& w, const auto& s) { input.OnScroll(s); });
  w->Show();
  RenderContextOptions rco{};
  OpenGL::RenderContextOpenGL ogl(*w, rco);
  while (!w->ShouldClose()) {
    w->PollEvent();
    input.Update(*w);
    if (input.GetKeyDown(Rad::KeyCode::A)) {
      Logger::Get()->info("yes");
    }
    if (input.GetKeyUp(Rad::KeyCode::A)) {
      Logger::Get()->info("oh~");
    }
  }
  w->Destroy();
  Window::Shutdown();
  return 0;
}
