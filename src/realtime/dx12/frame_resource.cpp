#include <rad/realtime/dx12/frame_resource.h>

namespace Rad::DX12 {

FrameResource::FrameResource(Device* device)
    : _device(device) {
  ThrowIfFailed(device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc)));
  ThrowIfFailed(device->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&_cmdList)));
  ThrowIfFailed(_cmdList->Close());
}

void FrameResource::Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence) {
  if (!_populated) {
    return;
  }
  queue->Signal(fence, _lastFenceIndex);
}

void FrameResource::Execute(ID3D12CommandQueue* queue, ID3D12Fence* fence, UINT64& fenceIndex) {
  if (!_populated) {
    return;
  }
  _lastFenceIndex = ++fenceIndex;
  ID3D12CommandList* ppCommandLists[] = {_cmdList.Get()};
  queue->ExecuteCommandLists(1, ppCommandLists);
}

void FrameResource::Sync(ID3D12Fence* fence) {
  if (!_populated || _lastFenceIndex == 0) {
    return;
  }
  if (fence->GetCompletedValue() < _lastFenceIndex) {
    LPCWSTR falseValue = 0;
    HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(fence->SetEventOnCompletion(_lastFenceIndex, eventHandle));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

ScopedCommandList FrameResource::Command() {
  _populated = true;
  return ScopedCommandList(_cmdList.Get(), _cmdAlloc.Get());
}

}  // namespace Rad::DX12
