#pragma once

#include "buffer.h"

namespace Rad::DX12 {

class ReadbackBuffer final : public Buffer {
 public:
  ReadbackBuffer(Device* device, UINT64 byteSize);
  ReadbackBuffer(ReadbackBuffer&&) = default;
  ReadbackBuffer(ReadbackBuffer const&) = delete;
  ~ReadbackBuffer() noexcept override;

  ID3D12Resource* GetResource() const override { return _resource.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override { return _resource->GetGPUVirtualAddress(); }
  UINT64 GetByteSize() const override { return _byteSize; }
  D3D12_RESOURCE_STATES GetInitState() const override { return D3D12_RESOURCE_STATE_GENERIC_READ; }

  void CopyData(UInt64 offset, void* data, UINT64 size) const;

 private:
  ComPtr<ID3D12Resource> _resource;
  UINT64 _byteSize;
};

}  // namespace Rad::DX12
