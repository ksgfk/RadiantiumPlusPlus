#pragma once

#include "utility.h"
#include "device.h"

namespace Rad::DX12 {

class FrameResource {
 private:
  Device* _device{nullptr};
  ComPtr<ID3D12CommandAllocator> _cmdAlloc;
  ComPtr<ID3D12GraphicsCommandList> _cmdList;
  UINT64 _lastFenceIndex{0};
};

}  // namespace Rad::DX12
