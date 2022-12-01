#include <rad/realtime/opengl/buffer.h>

namespace Rad::OpenGL {

Buffer::Buffer(GLContext* gl, GLsizei byteCount)
    : Resource(gl), _byteCount(byteCount) {
  RAD_CHECK_GL(_gl, CreateBuffers(1, &_buffer));
  RAD_CHECK_GL(_gl, NamedBufferStorage(_buffer, _byteCount, nullptr, 0));
}

Buffer::~Buffer() noexcept {
  if (_buffer != std::numeric_limits<GLuint>::max()) {
    RAD_CHECK_GL(_gl, DeleteBuffers(1, &_buffer));
    _buffer = std::numeric_limits<GLuint>::max();
  }
}

void Buffer::Write(GLintptr offset, const void* data, GLsizei size) const {
  RAD_CHECK_GL(_gl, NamedBufferSubData(_buffer, offset, size, data));
}

}  // namespace Rad::OpenGL
