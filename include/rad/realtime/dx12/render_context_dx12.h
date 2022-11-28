#pragma once

#include "utility.h"
#include "../render_context.h"
#include "../window.h"

#include <vector>

namespace Rad::DX12 {

class RenderContextDX12 : public RenderContext {
 public:
  RenderContextDX12(const Window& window, const RenderContextOptions& opts);
  RenderContextDX12(const RenderContextDX12&) = delete;
  RenderContextDX12(RenderContextDX12&&) noexcept;
  RenderContextDX12& operator=(const RenderContextDX12&) = delete;
  RenderContextDX12& operator=(RenderContextDX12&&) noexcept;
  ~RenderContextDX12() noexcept override;

  void Draw() override;

  static DXGI_FORMAT ConvertFormat(PixelFormat format);
  static DXGI_FORMAT ConvertFormat(DepthStencilFormat format);

 private:
  ComPtr<ID3D12Device> _device;
  ComPtr<ID3D12CommandQueue> _directCmdQueue;
  ComPtr<IDXGISwapChain3> _swapChain;
  UINT _frameCount{};
  UINT _nowFrameIndex{};
  ComPtr<ID3D12DescriptorHeap> _rtvHeap;
  UINT _rtvDescSize{};
  Vector2i _windowSize{};

  ComPtr<ID3D12CommandAllocator> _directCmdAlloc;
  ComPtr<ID3D12GraphicsCommandList> _directCmdList;
  std::vector<ComPtr<ID3D12Resource>> _rtv;
  ComPtr<ID3D12Fence> _fence;
  UINT64 _nowFence{};
};

}  // namespace Rad::DX12
