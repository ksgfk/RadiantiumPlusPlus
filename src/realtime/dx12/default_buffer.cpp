#include <rad/realtime/dx12/default_buffer.h>

namespace Rad::DX12 {

DefaultBuffer::DefaultBuffer(
    Device* device,
    UINT64 byteSize,
    D3D12_RESOURCE_STATES initState)
    : Buffer(device), _byteSize(byteSize), _initState(initState) {
  auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto buffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  ThrowIfFailed(device->GetDevice()->CreateCommittedResource(
      &prop,
      D3D12_HEAP_FLAG_NONE,
      &buffer,
      initState,
      nullptr,
      IID_PPV_ARGS(&_resource)));
}

DefaultBuffer::~DefaultBuffer() noexcept = default;

}  // namespace Rad::DX12
