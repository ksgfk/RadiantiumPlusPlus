#include <rad/realtime/window.h>
#include <rad/core/logger.h>

#include <rad/realtime/opengl/render_context_ogl.h>

using namespace Rad;

int main() {
  Window::Init();
  WindowOptions opts{Vector2i(1280, 720), "芜湖，起飞"};
  Unique<Window> w = Window::Create(opts);
  w->Show();
  RenderContextOptions rco{};
  OpenGL::RenderContextOpenGL ogl(*w, rco);
  while (!w->ShouldClose()) {
    w->PollEvent();
  }
  w->Destroy();
  Window::Shutdown();
  return 0;
}
