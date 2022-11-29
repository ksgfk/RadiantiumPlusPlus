#pragma once

#include "resource.h"

namespace Rad::DX12 {

class Buffer : public Resource {
 public:
  Buffer(Device* device) : Resource(device) {}
  virtual ~Buffer() noexcept override = default;
  Buffer(Buffer&&) noexcept = default;

  virtual D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const = 0;
  virtual UINT64 GetByteSize() const = 0;
};

}  // namespace Rad::DX12
