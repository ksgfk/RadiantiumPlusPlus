#pragma once

#include "buffer.h"

namespace Rad::DX12 {

class DefaultBuffer : public Buffer {
 public:
  DefaultBuffer(
      Device* device,
      UINT64 byteSize,
      D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON);
  DefaultBuffer(DefaultBuffer&&) = default;
  DefaultBuffer(DefaultBuffer const&) = delete;
  ~DefaultBuffer() noexcept override;

  ID3D12Resource* GetResource() const override { return _resource.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override { return _resource->GetGPUVirtualAddress(); }
  UINT64 GetByteSize() const override { return _byteSize; }
  D3D12_RESOURCE_STATES GetInitState() const override { return _initState; }

 private:
  UINT64 _byteSize;
  D3D12_RESOURCE_STATES _initState;
  ComPtr<ID3D12Resource> _resource;
};

}  // namespace Rad::DX12
