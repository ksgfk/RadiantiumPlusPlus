#include <rad/realtime/opengl/vertex_array.h>

namespace Rad::OpenGL {

VertexArray::VertexArray(
    GLContext* gl,
    const VertexArrayBingingDescriptor& desc)
    : Resource(gl) {
  RAD_CHECK_GL(_gl, CreateVertexArrays(1, &_vao));
  for (GLuint i = 0; i < desc.BufferCount; i++) {
    RAD_CHECK_GL(_gl, VertexArrayVertexBuffer(
                          _vao, i,
                          desc.BufferInfos[i].BufferObject->GetObject(),
                          desc.BufferInfos[i].Offset,
                          desc.BufferInfos[i].Stride));
  }
  if (desc.IndexBuffer != nullptr) {
    RAD_CHECK_GL(_gl, VertexArrayElementBuffer(_vao, desc.IndexBuffer->GetObject()));
  }
  for (GLuint i = 0; i < desc.VertexCount; i++) {
    RAD_CHECK_GL(_gl, EnableVertexArrayAttrib(_vao, i));
    RAD_CHECK_GL(_gl, VertexArrayAttribFormat(
                          _vao, i,
                          desc.VertexInfos[i].Size, desc.VertexInfos[i].Type,
                          GL_FALSE,
                          desc.VertexInfos[i].Relativeoffset));
    RAD_CHECK_GL(_gl, VertexArrayAttribBinding(_vao, i, desc.VertexInfos[i].BufferIndex));
  }
}

}  // namespace Rad::OpenGL
