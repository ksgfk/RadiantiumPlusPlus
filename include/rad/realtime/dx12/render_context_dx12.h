#pragma once

#include "utility.h"
#include "../render_context.h"
#include "../window.h"
#include <rad/core/logger.h>

#include "device.h"
#include "texture.h"
#include "frame_resource.h"

#include <vector>

namespace Rad::DX12 {

class RenderContextDX12 {
 public:
  RenderContextDX12(const Window& window, const RenderContextOptions& opts);

  void SyncCommandQueue();
  void Resize(const Vector2i&);
  void Render();

  static DXGI_FORMAT ConvertFormat(PixelFormat format);
  static DXGI_FORMAT ConvertFormat(DepthStencilFormat format);

 private:
  void PopulateCommandList(FrameResource& frame, UINT index);

  Share<spdlog::logger> _logger;
  HWND _hwnd;
  RenderContextOptions _opts;

  std::unique_ptr<Device> _device;
  ComPtr<IDXGISwapChain3> _swapChain;
  ComPtr<ID3D12CommandQueue> _commandQueue;

  ComPtr<ID3D12DescriptorHeap> _rtvHeap;
  ComPtr<ID3D12DescriptorHeap> _dsvHeap;
  UINT32 _dsvDescriptorSize;
  UINT32 _rtvDescriptorSize;

  std::vector<std::unique_ptr<Texture>> _renderTargets;
  std::vector<std::unique_ptr<Texture>> _depthTargets;
  std::vector<std::unique_ptr<FrameResource>> _frameResources;

  ComPtr<ID3D12Fence> _fence;
  UINT32 _backBufferIndex;
  UINT64 _fenceValue;
};

}  // namespace Rad::DX12
