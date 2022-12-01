#pragma once

#include "resource.h"

namespace Rad::OpenGL {

class Buffer final : public Resource {
 public:
  Buffer(GLContext* gl, GLsizei byteCount);
  ~Buffer() noexcept override;

  GLuint GetObject() const override { return _buffer; }
  void Write(GLintptr  offset, const void* data, GLsizei size) const;

 private:
  GLuint _buffer;
  GLsizei _byteCount;
};

}  // namespace Rad::OpenGL
