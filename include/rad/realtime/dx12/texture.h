#pragma once

#include "resource.h"

namespace Rad::DX12 {

enum class TextureDimension : UINT {
  None,
  Tex1D,
  Tex2D,
  Tex3D,
  Cubemap,
  Tex2DArray,
};

class Texture final : public Resource {
 public:
  enum class TextureUsage : UINT {
    None = 0,
    RenderTarget = 0x1,
    DepthStencil = 0x2,
    UnorderedAccess = 0x4,
    GenericColor = (UnorderedAccess | RenderTarget)
  };
  static constexpr float CLEAR_COLOR[4] = {0, 0, 0, 0};
  static constexpr float CLEAR_DEPTH = 1;
  static constexpr uint8_t CLEAR_STENCIL = 0;

  Texture(
      Device* device,
      UINT width,
      UINT height,
      DXGI_FORMAT format,
      TextureDimension dimension,
      UINT depth,
      UINT mip,
      TextureUsage usage,
      D3D12_RESOURCE_STATES resourceState);
  Texture(
      Device* device,
      IDXGISwapChain3* swapchain,
      UINT frame);
  ~Texture() noexcept override;

  ID3D12Resource* GetResource() const override { return _resource.Get(); }
  D3D12_RESOURCE_STATES GetInitState() const override { return _initState; }

  D3D12_SHADER_RESOURCE_VIEW_DESC GetColorSrvDesc(UINT mipOffset) const;
  D3D12_UNORDERED_ACCESS_VIEW_DESC GetColorUavDesc(UINT targetMipLevel) const;

 private:
  ComPtr<ID3D12Resource> _resource;
  D3D12_RESOURCE_STATES _initState;
  TextureDimension _dimension;
  TextureUsage _usage;
};

}  // namespace Rad::DX12
