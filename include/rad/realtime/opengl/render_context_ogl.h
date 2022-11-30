#pragma once

#include "utility.h"
#include "../render_context.h"
#include "../window.h"

#include <rad/core/logger.h>

namespace Rad::OpenGL {

class RenderContextOpenGL : public RenderContext {
 public:
  RenderContextOpenGL(const Window& window, const RenderContextOptions& opts);
  RenderContextOpenGL(const RenderContextOpenGL&) = delete;
  RenderContextOpenGL(RenderContextOpenGL&&) noexcept = default;
  RenderContextOpenGL& operator=(const RenderContextOpenGL&) = delete;
  RenderContextOpenGL& operator=(RenderContextOpenGL&&) noexcept = default;
  ~RenderContextOpenGL() noexcept override = default;

 private:
  Share<spdlog::logger> _logger;
  Unique<GladGLContext> _ctx;
};

}  // namespace Rad::OpenGL
