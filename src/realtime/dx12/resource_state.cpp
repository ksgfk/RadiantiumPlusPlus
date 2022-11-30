#include <rad/realtime/dx12/resource_state.h>

namespace Rad::DX12 {

void ResourceState::CollectState(const Resource* resource, D3D12_RESOURCE_STATES newState) {
  D3D12_RESOURCE_STATES initState = resource->GetInitState();
  auto [iter, has] = _resStates.try_emplace(resource, ResourceState::State{initState, newState});
  if (has) {
    ResourceState::State& st = iter->second;
    st.CurrentState = newState;
  }
}

void ResourceState::UpdateState(ID3D12GraphicsCommandList* cmdList) {
  for (auto&& i : _resStates) {
    auto&& [res, state] = i;
    if (state.CurrentState != state.LastState) {
      D3D12_RESOURCE_BARRIER& transBarrier = _tempBarriers.emplace_back();
      transBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      transBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      transBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      transBarrier.Transition.pResource = res->GetResource();
      transBarrier.Transition.StateBefore = state.LastState;
      transBarrier.Transition.StateAfter = state.CurrentState;
    }
  }
  if (!_tempBarriers.empty()) {
    cmdList->ResourceBarrier((UINT)_tempBarriers.size(), _tempBarriers.data());
    _tempBarriers.clear();
  }
}

void ResourceState::RestoreState(ID3D12GraphicsCommandList* cmdList) {
  for (auto&& i : _resStates) {
    auto&& [res, state] = i;
    if (state.CurrentState != state.LastState) {
      D3D12_RESOURCE_BARRIER& transBarrier = _tempBarriers.emplace_back();
      transBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      transBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      transBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      transBarrier.Transition.pResource = res->GetResource();
      transBarrier.Transition.StateBefore = state.CurrentState;
      transBarrier.Transition.StateAfter = state.LastState;
    }
  }
  if (!_tempBarriers.empty()) {
    cmdList->ResourceBarrier((UINT)_tempBarriers.size(), _tempBarriers.data());
    _tempBarriers.clear();
  }
  _resStates.clear();
}

bool ResourceState::IsWriteState(D3D12_RESOURCE_STATES state) {
  switch (state) {
    case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
    case D3D12_RESOURCE_STATE_COPY_DEST:
      return true;
    default:
      return false;
  }
}

}  // namespace Rad::DX12
