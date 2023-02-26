#pragma once

#if defined(_WIN32)
#include <windows.h>
#endif

#include <filesystem>
#include <unordered_map>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <imfilebrowser.h>

#include <rad/core/types.h>
#include <rad/core/logger.h>

struct GLFWwindow;

namespace Rad {

Matrix4f LookAt(const Vector3f& pos, const Vector3f& target, const Vector3f& up);
Matrix4f Perspective(float fovy, float aspect, float near, float far);
Matrix4f Ortho(float left, float right, float bottom, float top, float zNear, float zFar);

struct ImGuiRenderData {
  GLuint vao;
  GLuint vbo;
  GLuint ibo;
  GLuint uboPerFrame;
  GLuint shaderProgram;
  GLuint fontTex;
};

class Application {
 public:
  Application(int argc, char** argv);

  void Run();
  void LoadI18n(const std::filesystem::path& path);
  const char* I18n(const std::string& key) const;

 private:
  void Start();
  void Update();
  void Destroy();

  void InitGraphics();
  void InitImGui();
  void UpdateImGui();
  void DrawStartPass();
  void DrawImGuiPass();
  void GLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message) const;
  bool GLCompileShader(const char* source, GLenum type, GLuint* shader);
  bool GLLinkProgram(const GLuint* shader, int count, GLuint* program);

  void OnGui();
  void OnGuiMenuBar();
  void OnGuiMsgBox();

  void ShowMsgBox(const std::string& msg);
  bool IsMsgBoxOpen();

  Share<spdlog::logger> _logger;
  GLFWwindow* _window;
  ImGuiRenderData _imRender;

  std::unordered_map<std::string, std::string> _i18n;
  bool _hasWorkspace;
  std::filesystem::path _workRoot;
  Vector3f _backgroundColor;

  Unique<ImGui::FileBrowser> _menuBarFb;
  bool _isMenuBarNewSceneRecFbRes;

  bool _isShowMsgBox;
  std::string _msgBoxTitle;
  std::string _msgBoxText;
};

}  // namespace Rad
