#pragma once

#include "device.h"

namespace Rad::DX12 {

class Resource {
 public:
  Resource(Device* device) : _device(device) {}
  Resource(Resource&&) = default;
  Resource(Resource const&) = delete;
  virtual ~Resource() = default;

  Device* GetDevice() const { return _device; }
  virtual ID3D12Resource* GetResource() const { return nullptr; }
  virtual D3D12_RESOURCE_STATES GetInitState() const { return D3D12_RESOURCE_STATE_COMMON; }

 protected:
  Device* _device;
};

}  // namespace Rad::DX12
