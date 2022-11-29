#pragma once

#include "device.h"

namespace Rad::DX12 {

class ScopedCommandList {
 public:
  inline ScopedCommandList(
      ID3D12GraphicsCommandList* cmdList,
      ID3D12CommandAllocator* cmdAlloc) : _cmdList(cmdList) {
    ThrowIfFailed(cmdAlloc->Reset());
    ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));
  }
  inline ~ScopedCommandList() noexcept {
    if (_cmdList) {
      _cmdList->Close();
    }
  }
  ID3D12GraphicsCommandList* GetCommandList() const { return _cmdList; }

 private:
  ID3D12GraphicsCommandList* _cmdList;
};

class FrameResource {
 public:
  FrameResource(Device* device);

  void Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence);
  void Execute(ID3D12CommandQueue* queue, ID3D12Fence* fence, UINT64& fenceIndex);
  void Sync(ID3D12Fence* fence);

  ScopedCommandList Command();

 private:
  Device* _device{nullptr};
  ComPtr<ID3D12CommandAllocator> _cmdAlloc;
  ComPtr<ID3D12GraphicsCommandList> _cmdList;
  UINT64 _lastFenceIndex{0};
  bool _populated{false};
};

}  // namespace Rad::DX12
