#include <rad/realtime/dx12/upload_buffer.h>

namespace Rad::DX12 {

UploadBuffer::UploadBuffer(
    Device* device,
    UINT64 byteSize)
    : Buffer(device), _byteSize(byteSize) {
  auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto buffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  ThrowIfFailed(device->GetDevice()->CreateCommittedResource(
      &prop,
      D3D12_HEAP_FLAG_NONE,
      &buffer,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&_resource)));
}

UploadBuffer::~UploadBuffer() noexcept = default;

void UploadBuffer::CopyData(UInt64 offset, const void* data, UINT64 size) const {
  void* mappedPtr;
  D3D12_RANGE range;
  range.Begin = offset;
  range.End = std::min(_byteSize, offset + size);
  ThrowIfFailed(_resource->Map(0, &range, &mappedPtr));
  memcpy(reinterpret_cast<std::byte*>(mappedPtr) + offset, data, range.End - range.Begin);
  _resource->Unmap(0, &range);
}

}  // namespace Rad::DX12
