#pragma once

#include "utility.h"

namespace Rad::DX12 {

class Device {
 public:
  Device(bool enableDebug, bool findHP, bool useWrap);
  ~Device() noexcept = default;

  IDXGIAdapter1* GetAdapter() const { return _adapter.Get(); }
  ID3D12Device5* GetDevice() const { return _dxDevice.Get(); }
  IDXGIFactory6* GetDXGIFactory() const { return _dxgiFactory.Get(); }

 private:
  ComPtr<IDXGIAdapter1> _adapter;
  ComPtr<ID3D12Device5> _dxDevice;
  ComPtr<IDXGIFactory6> _dxgiFactory;
};

}  // namespace Rad::DX12
