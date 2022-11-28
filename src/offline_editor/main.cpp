#include <rad/realtime/window.h>
#include <rad/core/logger.h>
#include <rad/realtime/dx12/render_context_dx12.h>

using namespace Rad;
using namespace Rad::DX12;

int main() {
  WindowOptions opts{Vector2i(1280, 720), "芜湖，起飞"};
  Unique<Window> w = Window::Create(opts);
  w->Show();
  w->AddResizeListener([](auto& win, const auto& i) {
    Logger::Get()->info("{}", i);
  });
  RenderContextOptions rco{};
  rco.EnableDebug = true;
  rco.SwapChainRTCount = 2;
  rco.SwapChainRTFormat = PixelFormat::RGBA32;
  rco.SwapChainRTSize = opts.Size;
  rco.SwapChainMultiSample = 1;
  // rco.SwapChainDSFormat = DepthStencilFormat::R24G8;
  Unique<RenderContext> rc = std::make_unique<RenderContextDX12>(*w, rco);
  while (!w->ShouldClose()) {
    w->PollEvent();
    rc->Draw();
  }
  return 0;
}
