#include <rad/realtime/window.h>
#include <rad/core/logger.h>
#include <rad/realtime/dx12/render_context_dx12.h>

using namespace Rad;
using namespace Rad::DX12;

int main() {
  WindowOptions opts{Vector2i(1280, 720), "芜湖，起飞"};
  Unique<Window> w = Window::Create(opts);
  w->Show();
  RenderContextOptions rco{};
  rco.EnableDebug = true;
  rco.SwapChainRTCount = 2;
  rco.SwapChainRTFormat = PixelFormat::RGBA32;
  rco.SwapChainRTSize = opts.Size;
  rco.SwapChainMultiSample = 1;
  rco.SwapChainDSFormat = DepthStencilFormat::Depth32Float;
  Unique<RenderContextDX12> rc = std::make_unique<RenderContextDX12>(*w, rco);
  w->AddResizeListener([&](auto& w, auto s) {
    Logger::Get()->info("{}", s);
    rc->Resize(s);
  });
  while (!w->ShouldClose()) {
    w->PollEvent();
    rc->Render();
  }
  return 0;
}
