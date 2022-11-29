#include <rad/realtime/dx12/readback_buffer.h>

namespace Rad::DX12 {

ReadbackBuffer::ReadbackBuffer(
    Device* device,
    UINT64 byteSize)
    : Buffer(device), _byteSize(byteSize) {
  auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
  auto buffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  ThrowIfFailed(device->GetDevice()->CreateCommittedResource(
      &prop,
      D3D12_HEAP_FLAG_NONE,
      &buffer,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&_resource)));
}

ReadbackBuffer::~ReadbackBuffer() noexcept = default;

void ReadbackBuffer::CopyData(UInt64 offset, void* data, UINT64 size) const {
  void* mapPtr;
  D3D12_RANGE range;
  range.Begin = offset;
  range.End = std::min(_byteSize, offset + size);
  ThrowIfFailed(_resource->Map(0, &range, &mapPtr));
  memcpy(data, reinterpret_cast<std::byte*>(mapPtr) + offset, range.End - range.Begin);
  _resource->Unmap(0, nullptr);
}

}  // namespace Rad::DX12
