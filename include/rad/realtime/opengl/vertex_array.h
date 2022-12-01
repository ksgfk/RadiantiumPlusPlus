#pragma once

#include "resource.h"
#include "buffer.h"

namespace Rad::OpenGL {

struct VertexArrayElementLayout {
  GLenum Type;
  GLint Size;
  GLuint Relativeoffset;
  GLuint BufferIndex;
};

struct VertexArrayBufferLayout {
  Buffer* BufferObject;
  GLintptr Offset;
  GLsizei Stride;
};

struct VertexArrayBingingDescriptor {
  const VertexArrayElementLayout* VertexInfos;
  size_t VertexCount;
  const VertexArrayBufferLayout* BufferInfos;
  size_t BufferCount;
  const Buffer* IndexBuffer;
};

class VertexArray final : public Resource {
 public:
  VertexArray(GLContext* gl, const VertexArrayBingingDescriptor& desc);
  ~VertexArray() noexcept override;

  GLuint GetObject() const override { return _vao; }

 private:
  GLuint _vao;
};

}  // namespace Rad::OpenGL
