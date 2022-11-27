#include <rad/editor/window.h>
#include <rad/core/logger.h>

using namespace Rad;
using namespace Rad::Editor;

int main() {
  WindowOptions opts{Vector2i(1280, 720), "芜湖，起飞"};
  Unique<Window> w = Window::Create(opts);
  w->Show();
  w->AddResizeListener([](auto& win, const auto& i) {
    Logger::Get()->info("{}", i);
  });
  while (!w->ShouldClose()) {
    w->PollEvent();
  }
  return 0;
}
