#pragma once

#include <rad/core/types.h>

namespace Rad::Editor {

enum class PixelFormat {
  RGBA32
};

enum class DepthStencilFormat{
  R24G8
};

struct RenderContextOptions {
  bool EnableDebug{false};
  Vector2i SwapChainRTSize;
  UInt32 SwapChainRTCount;
  PixelFormat SwapChainRTFormat;
  UInt32 SwapChainMultiSample;
  DepthStencilFormat SwapChainDSFormat;
};

class RenderContext {
 public:
  RenderContext() = default;
  RenderContext(const RenderContext&) = delete;
  RenderContext(RenderContext&&) noexcept = default;
  RenderContext& operator=(const RenderContext&) = delete;
  RenderContext& operator=(RenderContext&&) noexcept = default;
  virtual ~RenderContext() noexcept = default;

  virtual void Draw() = 0;
};

}  // namespace Rad::Editor
