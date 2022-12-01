#pragma once

#include "utility.h"

namespace Rad::OpenGL {

class Resource {
 public:
  Resource(GLContext* gl) : _gl(gl) {}
  Resource(Resource&&) noexcept = default;
  Resource(Resource const&) = delete;
  virtual ~Resource() noexcept = default;

  GLContext* GetGL() const { return _gl; }
  virtual GLuint GetObject() const { return std::numeric_limits<GLuint>::max(); }

 protected:
  GLContext* _gl;
};

}  // namespace Rad::OpenGL
