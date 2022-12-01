#pragma once

#include "resource.h"

#include <string>

namespace Rad::OpenGL {

class ShaderStage final : public Resource {
 public:
  ShaderStage(GLContext* gl, GLenum shaderType, const std::string& src);
  ~ShaderStage() noexcept override;

  GLuint GetObject() const override { return _shader; }
  GLenum GetShaderType() const { return _type; }

 private:
  GLuint _shader;
  GLenum _type;
};

class ShaderProgram final : public Resource {
 public:
  ShaderProgram(GLContext* gl, const ShaderStage& vs, const ShaderStage& fs);
  ~ShaderProgram() noexcept override;

  GLuint GetObject() const override { return _program; }

 private:
  GLuint _program;
};

}  // namespace Rad::OpenGL
