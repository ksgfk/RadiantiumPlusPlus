#include <rad/realtime/opengl/shader.h>

namespace Rad::OpenGL {

ShaderStage::ShaderStage(
    GLContext* gl,
    GLenum shaderType,
    const std::string& src)
    : Resource(gl) {
  _shader = RAD_CHECK_GL(_gl, CreateShader(shaderType));
  _type = shaderType;
  const char* str = src.c_str();
  GLint length = (GLint)src.size();
  RAD_CHECK_GL(_gl, ShaderSource(_shader, 1, &str, &length));
  RAD_CHECK_GL(_gl, CompileShader(_shader));
  GLint status;
  RAD_CHECK_GL(_gl, GetShaderiv(_shader, GL_COMPILE_STATUS, &status));
  bool isSuccess = status == GL_TRUE;
  if (!isSuccess) {
    GLint errorLen;
    RAD_CHECK_GL(_gl, GetShaderiv(_shader, GL_INFO_LOG_LENGTH, &errorLen));
    std::string errorInfo(errorLen, '\0');
    RAD_CHECK_GL(_gl, GetShaderInfoLog(_shader, errorLen, nullptr, errorInfo.data()));
    RAD_CHECK_GL(_gl, DeleteShader(_shader));
    _shader = std::numeric_limits<GLint>::max();
    throw RadOpenGLException("shader编译失败, 原因:\n{}", errorInfo);
  }
}

ShaderStage::~ShaderStage() noexcept {
  if (_shader != std::numeric_limits<GLint>::max()) {
    RAD_CHECK_GL(_gl, DeleteShader(_shader));
    _shader = std::numeric_limits<GLint>::max();
  }
}

ShaderProgram::ShaderProgram(
    GLContext* gl,
    const ShaderStage& vs,
    const ShaderStage& fs)
    : Resource(gl) {
  if (vs.GetShaderType() != GL_VERTEX_SHADER) {
    throw RadArgumentException("vs参数应该输入顶点着色器");
  }
  if (fs.GetShaderType() != GL_FRAGMENT_SHADER) {
    throw RadArgumentException("fs参数应该输入片元着色器");
  }
  _program = RAD_CHECK_GL(_gl, CreateProgram());
  RAD_CHECK_GL(_gl, AttachShader(_program, vs.GetObject()));
  RAD_CHECK_GL(_gl, AttachShader(_program, fs.GetObject()));
  RAD_CHECK_GL(_gl, LinkProgram(_program));
  GLint status;
  RAD_CHECK_GL(_gl, GetProgramiv(_program, GL_LINK_STATUS, &status));
  bool isSuccess = status == GL_TRUE;
  if (!isSuccess) {
    GLint errorLen;
    RAD_CHECK_GL(_gl, GetProgramiv(_program, GL_INFO_LOG_LENGTH, &errorLen));
    std::string errorInfo(errorLen, '\0');
    RAD_CHECK_GL(_gl, GetProgramInfoLog(_program, errorLen, nullptr, errorInfo.data()));
    RAD_CHECK_GL(_gl, DeleteProgram(_program));
    _program = std::numeric_limits<GLint>::max();
    throw RadOpenGLException("shader program链接失败, 原因:\n{}", errorInfo);
  }
}

ShaderProgram::~ShaderProgram() noexcept {
  if (_program != std::numeric_limits<GLint>::max()) {
    RAD_CHECK_GL(_gl, DeleteProgram(_program));
    _program = std::numeric_limits<GLint>::max();
  }
}

}  // namespace Rad::OpenGL
