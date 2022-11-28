#include <rad/realtime/dx12/device.h>

namespace Rad::DX12 {

Device::Device(bool enableDebug, bool findHP, bool useWrap) {
  uint32_t dxgiFactoryFlags = 0;
  if (enableDebug) {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
      debugController->EnableDebugLayer();
      dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
  }
  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_dxgiFactory)));
  bool adapterFound = false;
  for (UINT adapterIndex = 0;; adapterIndex++) {
    HRESULT hr1 = _dxgiFactory->EnumAdapterByGpuPreference(
        adapterIndex,
        findHP == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
        IID_PPV_ARGS(&_adapter));
    if (!hr1) {
      break;
    }
    if (!useWrap) {
      DXGI_ADAPTER_DESC1 desc;
      _adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }
    }
    HRESULT hr2 = D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr);
    if (SUCCEEDED(hr2)) {
      adapterFound = true;
      break;
    }
  }
  if (adapterFound) {
    D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dxDevice));
  } else {
    throw RadDX12Exception("无法找到适合的adapter");
  }
}

}  // namespace Rad::DX12
