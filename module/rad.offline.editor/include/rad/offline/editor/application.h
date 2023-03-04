#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <optional>

#include <glad/gl.h>

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/core/factory.h>
#include <rad/core/asset.h>

#include "gui_object.h"

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

  Share<spdlog::logger> GetLogger() const { return _logger; }
  Vector3f& GetBackgroundColor() { return _backgroundColor; }
  const std::filesystem::path& GetWorkRoot() const { return _workRoot; }
  bool HasWorkspace() const { return _hasWorkspace; }

  void AddGui(Unique<GuiObject> ui);
  void NewScene(const std::filesystem::path& sceneFile);
  std::optional<GuiObject*> FindUi(const std::string& name);

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

  void InitBasicGuiObject();
  void OnGui();

  void ExecuteRequestNewScene();

  Share<spdlog::logger> _logger;
  GLFWwindow* _window;
  ImGuiRenderData _imRender;
  std::vector<Unique<GuiObject>> _firstUseGui;
  std::vector<Unique<GuiObject>> _activeGui;
  std::vector<Unique<GuiObject>> _iterGuiCache;
  std::unordered_map<std::string, std::string> _i18n;
  bool _hasWorkspace{false};
  bool _hasRequestNewScene{false};
  std::filesystem::path _requestNewScenePath;
  std::filesystem::path _workRoot;
  std::string _sceneName;
  Vector3f _backgroundColor;
};

}  // namespace Rad
