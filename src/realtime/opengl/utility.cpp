#include <rad/realtime/opengl/utility.h>

#include <rad/core/logger.h>

namespace Rad::OpenGL {

void UtilCheckGLError(
    const GLContext* ctx,
    const char* callFuncName,
    const char* fileName, int line) {
  GLenum err = ctx->GetError();
  if (err == GL_NO_ERROR) {
    return;
  }
  auto logger = Logger::GetCategory("OpenGL");
  logger->error("error at {}, line: {}", fileName, line);
}

}  // namespace Rad::OpenGL
