#pragma once

#include "resource.h"

namespace Rad::DX12 {

class ResourceState {
 public:
  void CollectState(const Resource* resource, D3D12_RESOURCE_STATES newState);
  void UpdateState(ID3D12GraphicsCommandList* cmdList);
  void RestoreState(ID3D12GraphicsCommandList* cmdList);

  static bool IsWriteState(D3D12_RESOURCE_STATES state);

 private:
  struct State {
    D3D12_RESOURCE_STATES LastState;
    D3D12_RESOURCE_STATES CurrentState;
  };

  std::unordered_map<const Resource*, State> _resStates;
  std::vector<D3D12_RESOURCE_BARRIER> _tempBarriers;
};

}  // namespace Rad::DX12
