#pragma once

#include "../../core/types.h"

#if !defined(UNICODE)
#define UNICODE
#endif

#include <directx/d3dx12.h>

#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                                      \
  {                                                                                           \
    HRESULT result_m_ = (x);                                                                  \
    if (FAILED(result_m_)) {                                                                  \
      throw RadDX12Exception("err code {}, at: {}, line: {}", result_m_, __FILE__, __LINE__); \
    }                                                                                         \
  }
#endif

namespace Rad::DX12 {

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class RadDX12Exception : public RadException {
 public:
  template <typename... Args>
  RadDX12Exception(const char* fmt, const Args&... args) : RadException("RadDX12Exception", fmt, args...) {}
};

}  // namespace Rad::DX12
