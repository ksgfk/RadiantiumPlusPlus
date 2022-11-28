#pragma once

#include "utility.h"
#include "dx12_device.h"

namespace Rad::DX12 {

class FrameResource {
 private:
  Dx12Device* _device{nullptr};
  ComPtr<ID3D12CommandAllocator> _cmdAlloc;
  ComPtr<ID3D12GraphicsCommandList> _cmdList;
  UINT64 _lastFenceIndex{0};
};

}  // namespace Rad::DX12
