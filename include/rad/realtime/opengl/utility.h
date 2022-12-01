#pragma once

#include <rad/core/types.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#if defined(RAD_DEFINE_DEBUG)
#define RAD_CHECK_GL(gl, func) \
  gl->func;                    \
  UtilCheckGLError(gl, #func, __FILE__, __LINE__);
#else
#define RAD_CHECK_GL(gl, func) gl->func
#endif

namespace Rad::OpenGL {

using GLContext = GladGLContext;

class RadOpenGLException : public RadException {
 public:
  template <typename... Args>
  RadOpenGLException(const char* fmt, const Args&... args) : RadException("RadOpenGLException", fmt, args...) {}
};

void UtilCheckGLError(
    const GLContext* ctx,
    const char* callFuncName,
    const char* fileName, int line);

}  // namespace Rad::OpenGL
