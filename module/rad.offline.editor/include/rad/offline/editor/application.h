#pragma once

#include <filesystem>
#include <unordered_map>
#include <map>
#include <queue>
#include <list>
#include <vector>
#include <random>
#include <optional>

#include <glad/gl.h>

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/core/factory.h>
#include <rad/core/asset.h>
#include <rad/offline/render/renderer.h>

#include "gui_object.h"

struct GLFWwindow;

namespace Rad {

Matrix4f LookAt(const Vector3f& pos, const Vector3f& target, const Vector3f& up);
Matrix4f Perspective(float fovy, float aspect, float near, float far);
Matrix4f Ortho(float left, float right, float bottom, float top, float zNear, float zFar);
std::tuple<Vector3f, Eigen::Quaternionf, Vector3f> DecomposeTransform(const Matrix4f&);
Matrix3f GetMat3(const nlohmann::json&);
Matrix4f GetMat4(const nlohmann::json&);
Vector2f GetVec2(const nlohmann::json&);
Vector3f GetVec3(const nlohmann::json&);
const char** AssetNames();
size_t AssetNameCount();
const char** RendererNames();
size_t RendererNameCount();

struct ImGuiRenderData {
  GLuint vao;
  GLuint vbo;
  GLuint ibo;
  GLuint uboPerFrame;
  GLuint shaderProgram;
  GLuint fontTex;
};

struct GLVertex {
  Vector3f Position;
  Vector3f Normal;
  Vector2f UV;
};

struct PerFrameData {
  Matrix4f Model;
  Matrix4f View;
  Matrix4f Persp;
  Matrix4f MVP;
  Matrix4f InvModel;
};

struct PreviewRenderData {
  GLuint Fbo{0};
  GLuint ColorTex{0};
  GLuint Rbo{0};
  int Width{0};
  int Height{0};
  PerFrameData PerFrame{};
  GLuint ShaderProgram{0};
  GLuint UboPerFrame{0};
};

struct OfflineRenderData {
  GLuint ResultTex{0};
  GLuint CS{0};
  Unique<Renderer> Renderer;
  int Width{0};
  int Height{0};
};

class EditorAsset {
 public:
  EditorAsset(int type);
  virtual ~EditorAsset() noexcept = default;

  virtual std::pair<bool, std::string> Load(Application*) = 0;

  std::string Name;
  std::filesystem::path Loaction;
  int Type;
  int Reference;
};

class ObjMeshAsset : public EditorAsset {
 public:
  ObjMeshAsset();
  ~ObjMeshAsset() noexcept override;

  std::pair<bool, std::string> Load(Application*) override;

  GLuint VAO;
  GLuint VBO;
  GLuint IBO;
  int Indices;
};

class EditorAssetGuard {
 public:
  explicit EditorAssetGuard(EditorAsset*);
  EditorAssetGuard(const EditorAssetGuard&);
  EditorAssetGuard(EditorAssetGuard&&);
  EditorAssetGuard& operator=(const EditorAssetGuard&);
  EditorAssetGuard& operator=(EditorAssetGuard&&);
  EditorAssetGuard& operator=(nullptr_t);
  ~EditorAssetGuard() noexcept;

  EditorAsset* Ptr{nullptr};
};

struct BuiltinGeo {
  GLuint VAO;
  GLuint VBO;
  GLuint IBO;
  int Indices;
  std::string Name;
};

struct BuiltinShapes {
  BuiltinGeo Sphere;
  BuiltinGeo Cube;
  BuiltinGeo Rect;
};

class ShapeNode {
 public:
  ShapeNode& AddChildBefore(const ShapeNode& pos, ShapeNode&& node);
  ShapeNode& AddChildAfter(const ShapeNode& pos, ShapeNode&& node);
  ShapeNode& AddChildLast(ShapeNode&& node);
  void RemoveChild(ShapeNode& child);
  void SetToWorldMatrix(const Matrix4f& toWorld);
  std::optional<ShapeNode*> PreviousNode();
  std::optional<ShapeNode*> NextNode();

  Application* App{nullptr};
  size_t Id{0};
  std::string Name;
  Vector3f Position{Vector3f::Zero()};
  Vector3f Scale{Vector3f::Constant(1)};
  Eigen::Quaternionf Rotation{Eigen::Quaternionf::Identity()};
  Matrix4f ToWorld{Matrix4f::Identity()};
  Matrix4f ToLocal{Matrix4f::Identity()};

  ShapeNode* Parent{nullptr};
  std::list<ShapeNode> Children;
  std::list<ShapeNode>::iterator InParentIter{};

  EditorAssetGuard ShapeAsset{nullptr};
  BuiltinGeo* ShapeBuildin{nullptr};
  nlohmann::json Config{};

 private:
  void AfterAddChild(std::list<ShapeNode>::iterator&);
};

class PerspCamera {
 public:
  Vector3f Position{0, 0, 5};
  Eigen::Quaternionf Rotation{Eigen::Quaternionf::Identity()};
  Vector3f Target{Vector3f::Zero()};
  Vector3f Up{0, 1, 0};
  Matrix4f ToView{Matrix4f::Identity()};
  float Fovy{30.0f};
  float NearZ{0.001f};
  float FarZ{100.0f};
  Matrix4f ToPersp{Matrix4f::Identity()};
  Vector2i Resolution{1280, 720};
};

class DefaultMessageBox : public GuiObject {
 public:
  explicit DefaultMessageBox(Application* app, const std::string& msg);
  ~DefaultMessageBox() noexcept override = default;

  void OnStart() override;
  void OnGui() override;

  std::string Message;
  std::function<void(Application*)> OnOk;
  std::function<void(Application*)> OnCancel;
};

class LateUpdateCallback {
 public:
  LateUpdateCallback(int id, std::function<void(Application*)>&& callback);
  ~LateUpdateCallback() noexcept = default;

  int Id;
  std::function<void(Application*)> Callback;
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
  const FactoryManager& GetFactoryManager() { return *_factories; }
  const std::map<std::string, std::unique_ptr<EditorAsset>>& GetAssets() { return _nameToAsset; }
  ShapeNode& GetRoot() { return *_root; }
  PerspCamera& GetCamera() { return _camera; }
  std::mt19937& GetRng() { return _rng; }
  const PreviewRenderData& GetPreviewFb() { return _prevFbo; }
  std::array<BuiltinGeo*, 3> GetBuiltinShapes() { return {&_builtinShape.Sphere, &_builtinShape.Cube, &_builtinShape.Rect}; }
  nlohmann::json& GetWorkspaceConfig() { return _workConfig; }
  bool IsOfflineRendering() { return _isOfflineRendering; }
  OfflineRenderData& GetOfflineRender() { return _offlineRenderingData; }

  void AddGui(Unique<GuiObject> ui);
  void NewScene(const std::filesystem::path& sceneFile);
  void OpenScene(const std::filesystem::path& sceneFile);
  void SaveScene();
  void CloseScene();
  void RenderWorkspace();
  std::optional<GuiObject*> FindUi(const std::string& name);
  std::pair<bool, std::string> LoadAsset(const std::string& name, const std::filesystem::path& loaction, int type);
  ShapeNode NewNode();
  void AddLateUpdate(const LateUpdateCallback& cb);
  void ChangePreviewResolution();
  void StartOfflineRender();
  void StopOfflineRender();

 private:
  void Start();
  void Update();
  void Destroy();

  void InitGraphics();
  void InitImGui();
  void InitPreviewFrameBuffer(int width, int height);
  void InitBuiltinShapes();
  void InitOfflineRenderData();
  void UpdateImGui();
  void DrawStartPass();
  void DrawItemPass();
  void DrawImGuiPass();
  void DispatchToSrgb();
  void GLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message) const;
  bool GLCompileShader(const char* source, GLenum type, GLuint* shader);
  bool GLLinkProgram(const GLuint* shader, int count, GLuint* program);

  void InitBasicGuiObject();
  void OnGui();
  void OnLateUpdate();

  void CollectRenderItem();

  void BuildScene(const nlohmann::json&);

  void CreateWorkspace(const std::filesystem::path& sceneFile, bool isBuild);
  void ClearWorkspace();

  Share<spdlog::logger> _logger;
  GLFWwindow* _window;
  ImGuiRenderData _imRender;
  std::vector<Unique<GuiObject>> _firstUseGui;
  std::vector<Unique<GuiObject>> _activeGui;
  std::vector<Unique<GuiObject>> _iterGuiCache;
  std::unordered_map<std::string, std::string> _i18n;
  std::unique_ptr<FactoryManager> _factories;
  bool _hasWorkspace{false};
  std::filesystem::path _workRoot;
  std::string _sceneName;
  nlohmann::json _workConfig;
  Vector3f _backgroundColor;
  std::map<std::string, std::unique_ptr<EditorAsset>> _nameToAsset;
  Unique<ShapeNode> _root;
  size_t _nodeIdPool;
  std::queue<ShapeNode*> _recTemp;
  std::vector<ShapeNode*> _renderItems;
  PerspCamera _camera;
  std::mt19937 _rng;
  PreviewRenderData _prevFbo{};
  BuiltinShapes _builtinShape;
  std::vector<LateUpdateCallback> _lateUpdate;
  bool _isOfflineRendering{false};
  OfflineRenderData _offlineRenderingData;
};

TriangleModel CreateSphere(float radius, int numberSlices);
TriangleModel CreateCube(float halfExtend);
TriangleModel CreateRect(float halfExtend);

}  // namespace Rad
