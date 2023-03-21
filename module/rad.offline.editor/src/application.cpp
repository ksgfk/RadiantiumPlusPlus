#include <rad/offline/editor/application.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <thread>
#include <string>
#include <algorithm>
#include <fstream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <nlohmann/json.hpp>

#include <rad/core/wavefront_obj_reader.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/editor/gui_main_bar.h>
#include <rad/offline/editor/gui_asset_panel.h>
#include <rad/offline/editor/gui_hierarchy.h>
#include <rad/offline/editor/gui_camera.h>
#include <rad/offline/editor/gui_preview_scene.h>
#include <rad/offline/editor/gui_offline_render_panel.h>

namespace Rad {

constexpr const float PI = 3.1415926f;

const char* imguiVertShader = R"(
#version 450 core
layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;
layout (std140, binding = 0) uniform PerFrameData {
  mat4 MVP;
} _PerFrame;
out vec2 Frag_UV;
out vec4 Frag_Color;
void main() {
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = _PerFrame.MVP * vec4(Position.xy,0,1);
}
)";
const char* imguiFragShader = R"(
#version 450 core
in vec2 Frag_UV;
in vec4 Frag_Color;
layout (binding = 0) uniform sampler2D Texture;
layout (location = 0) out vec4 out_Color;
void main() {
  out_Color = Frag_Color * texture(Texture, Frag_UV.st);
}
)";

const char* normalVert = R"(
#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_UV;
layout (std140, binding = 0) uniform PerFrameData {
  mat4 Model;
  mat4 View;
  mat4 Persp;
  mat4 MVP;
  mat4 InvModel;
} _PerFrame;
out vec3 v_worldNormal;
void main() {
  gl_Position = _PerFrame.MVP * vec4(a_Position, 1);
  v_worldNormal = transpose(mat3(_PerFrame.InvModel)) * a_Normal;
}
)";
const char* normalFrag = R"(
#version 450 core
in vec3 v_worldNormal;
layout (location = 0) out vec4 out_Color;
void main() {
  vec3 normal = normalize(v_worldNormal);
  vec3 color = (normal + vec3(1.0)) * vec3(0.5);
  out_Color = vec4(color, 1.0);
}
)";

Matrix4f LookAt(const Vector3f& eye, const Vector3f& target, const Vector3f& up) {
  Eigen::Vector3f f(target - eye);
  Eigen::Vector3f s(f.cross(up));
  Eigen::Vector3f u(s.cross(f));
  f.normalize();
  s.normalize();
  u.normalize();

  Eigen::Matrix4f mat = Eigen::Matrix4f::Identity();
  mat(0, 0) = s.x();
  mat(0, 1) = s.y();
  mat(0, 2) = s.z();
  mat(1, 0) = u.x();
  mat(1, 1) = u.y();
  mat(1, 2) = u.z();
  mat(2, 0) = -f.x();
  mat(2, 1) = -f.y();
  mat(2, 2) = -f.z();
  mat(0, 3) = -s.dot(eye);
  mat(1, 3) = -u.dot(eye);
  mat(2, 3) = f.dot(eye);
  return mat;
}

Matrix4f Perspective(float fovy, float aspect, float zNear, float zFar) {
  Matrix4f tr = Matrix4f::Zero();
  float radf = fovy * PI / 180.0f;
  float tanHalfFovy = std::tan(radf / 2.0f);
  tr(0, 0) = 1.0f / (aspect * tanHalfFovy);
  tr(1, 1) = 1.0f / (tanHalfFovy);
  tr(2, 2) = -(zFar + zNear) / (zFar - zNear);
  tr(3, 2) = -1.0f;
  tr(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
  return tr;
}

Matrix4f Ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
  Matrix4f mat = Matrix4f::Identity();
  mat(0, 0) = float(2) / (right - left);
  mat(1, 1) = float(2) / (top - bottom);
  mat(2, 2) = -float(2) / (zFar - zNear);
  mat(3, 0) = -(right + left) / (right - left);
  mat(3, 1) = -(top + bottom) / (top - bottom);
  mat(3, 2) = -(zFar + zNear) / (zFar - zNear);
  return mat;
}

std::tuple<Vector3f, Eigen::Quaternionf, Vector3f> DecomposeTransform(const Matrix4f& m) {
  Eigen::Affine3f aff(m);
  Vector3f t = aff.translation();
  Matrix3f ro, sc;
  aff.computeRotationScaling(&ro, &sc);
  Eigen::Quaternionf q(ro);
  return {t, q, sc.diagonal()};
}

Matrix3f GetMat3(const nlohmann::json& data) {
  Matrix3f mat;
  int k = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      mat(i, j) = data[k].get<float>();
      k++;
    }
  }
  return mat;
}

Matrix4f GetMat4(const nlohmann::json& data) {
  Matrix4f mat;
  int k = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat(i, j) = data[k].get<float>();
      k++;
    }
  }
  return mat;
}

Vector2f GetVec2(const nlohmann::json& data) {
  Vector2f vec;
  for (int i = 0; i < 2; i++) {
    vec[i] = data[i];
  }
  return vec;
}

Vector3f GetVec3(const nlohmann::json& data) {
  Vector3f vec;
  for (int i = 0; i < 3; i++) {
    vec[i] = data[i];
  }
  return vec;
}

const char* _assetNamesArray[] = {
    "Wavefront Obj Model",
    "OpenEXR Image",
    "PNG,JPG,TGA...etc"};
const char** AssetNames() {
  return _assetNamesArray;
}
size_t AssetNameCount() {
  return Math::ArrayLength(_assetNamesArray);
}

const char* _renderAlgosArray[] = {
    "ao",
    "path",
    "ptracer",
    "bdpt"};
const char** RendererNames() {
  return _renderAlgosArray;
}
size_t RendererNameCount() {
  return 4;
}

EditorAsset::EditorAsset(int type) : Type(type), Reference(0) {}

ObjMeshAsset::ObjMeshAsset() : EditorAsset(0), VAO(0), VBO(0), IBO(0) {}

ObjMeshAsset::~ObjMeshAsset() noexcept {
  if (VAO != 0) {
    glDeleteVertexArrays(1, &VAO);
    VAO = 0;
    GLuint v[] = {VBO, IBO};
    glDeleteBuffers(2, v);
    VBO = 0;
    IBO = 0;
  }
}

EditorAssetGuard::EditorAssetGuard(EditorAsset* ptr) : Ptr(ptr) {
  if (Ptr != nullptr) {
    Ptr->Reference++;
  }
}

EditorAssetGuard::EditorAssetGuard(const EditorAssetGuard& other) : Ptr(other.Ptr) {
  if (Ptr != nullptr) {
    Ptr->Reference++;
  }
}

EditorAssetGuard::EditorAssetGuard(EditorAssetGuard&& other) {
  if (Ptr != nullptr) {
    Ptr->Reference--;
  }
  Ptr = other.Ptr;
  other.Ptr = nullptr;
}

EditorAssetGuard& EditorAssetGuard::operator=(const EditorAssetGuard& other) {
  Ptr = other.Ptr;
  if (Ptr != nullptr) {
    Ptr->Reference++;
  }
  return *this;
}

EditorAssetGuard& EditorAssetGuard::operator=(EditorAssetGuard&& other) {
  if (Ptr != nullptr) {
    Ptr->Reference--;
  }
  Ptr = other.Ptr;
  other.Ptr = nullptr;
  return *this;
}

EditorAssetGuard& EditorAssetGuard::operator=(nullptr_t) {
  if (Ptr != nullptr) {
    Ptr->Reference--;
    Ptr = nullptr;
  }
  return *this;
}

EditorAssetGuard::~EditorAssetGuard() noexcept {
  if (Ptr != nullptr) {
    Ptr->Reference--;
    Ptr = nullptr;
  }
}

static std::tuple<GLuint, GLuint, GLuint, int> UploadToGpu(TriangleModel& model) {
  if (!model.HasNormal()) {
    model.AllocNormal();
    // 不是很好的法线计算方法, 但应该够用
    std::vector<int> cnt;
    cnt.resize(model.VertexCount());
    auto pt = model.GetPosition();
    auto nt = model.GetNormal();
    for (uint32_t i = 0; i < model.VertexCount(); i++) {
      nt[i] = Vector3f::Zero();
    }
    auto ind = model.GetIndices();
    for (uint32_t i = 0; i < model.IndexCount(); i += 3) {
      auto px = pt[ind[i]];
      auto py = pt[ind[i + 1]];
      auto pz = pt[ind[i + 2]];
      auto n = (px - py).cross(px - pz);
      nt[ind[i]] += n;
      nt[ind[i + 1]] += n;
      nt[ind[i + 2]] += n;
      cnt[ind[i]]++;
      cnt[ind[i + 1]]++;
      cnt[ind[i + 2]]++;
    }
    for (uint32_t i = 0; i < model.VertexCount(); i++) {
      nt[i] = (nt[i] / cnt[i]).normalized();
    }
  }
  if (!model.HasUV()) {
    model.AllocUV();
    for (uint32_t i = 0; i < model.VertexCount(); i++) {
      model.GetUV()[i] = Vector2f{};
    }
  }
  std::vector<GLVertex> vertices;
  vertices.reserve(model.VertexCount());
  for (size_t i = 0; i < model.VertexCount(); i++) {
    vertices.emplace_back(GLVertex{model.GetPosition()[i], model.GetNormal()[i], model.GetUV()[i]});
  }
  const auto& indices = model.GetIndices();
  GLuint vao;
  glCreateVertexArrays(1, &vao);
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, sizeof(GLVertex) * vertices.size(), vertices.data(), 0);
  GLuint ibo;
  glCreateBuffers(1, &ibo);
  glNamedBufferStorage(ibo, sizeof(UInt32) * model.IndexCount(), indices.get(), 0);
  glVertexArrayElementBuffer(vao, ibo);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(GLVertex));
  glEnableVertexArrayAttrib(vao, 0);
  glEnableVertexArrayAttrib(vao, 1);
  glEnableVertexArrayAttrib(vao, 2);
  glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(GLVertex, Position));
  glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(GLVertex, Normal));
  glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(GLVertex, UV));
  glVertexArrayAttribBinding(vao, 0, 0);
  glVertexArrayAttribBinding(vao, 1, 0);
  glVertexArrayAttribBinding(vao, 2, 0);
  return {vao, vbo, ibo, model.IndexCount()};
}

std::pair<bool, std::string> ObjMeshAsset::Load(Application* app) {
  WavefrontObjReader reader(std::make_unique<std::ifstream>(app->GetWorkRoot() / Loaction));
  reader.Read();
  auto model = reader.ToModel();
  std::pair<bool, std::string> result;
  if (reader.HasError()) {
    result.first = false;
    result.second = reader.Error();
  } else {
    std::tie(VAO, VBO, IBO, Indices) = UploadToGpu(model);
    result.first = true;
  }
  return result;
}

ShapeNode& ShapeNode::AddChildBefore(const ShapeNode& pos, ShapeNode&& node) {
  ShapeNode temp = std::move(node);
  if (temp.Parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto childIter = Children.emplace(pos.InParentIter, std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

ShapeNode& ShapeNode::AddChildAfter(const ShapeNode& pos, ShapeNode&& node) {
  ShapeNode temp = std::move(node);
  if (temp.Parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto iter = pos.InParentIter;
  auto childIter = Children.emplace(++iter, std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

ShapeNode& ShapeNode::AddChildLast(ShapeNode&& node) {
  ShapeNode temp = std::move(node);
  if (temp.Parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto childIter = Children.emplace(Children.end(), std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

void ShapeNode::RemoveChild(ShapeNode& child) {
  if (child.Parent != this) {
    throw RadArgumentException("parent and child not same!");
  }
  Children.erase(child.InParentIter);
  child.InParentIter = {};
  child.Parent = nullptr;
}

std::optional<ShapeNode*> ShapeNode::PreviousNode() {
  auto first = Parent->Children.begin();
  if (InParentIter == first) {
    return std::nullopt;
  }
  auto prev = InParentIter;
  prev--;
  return std::make_optional(&(*(prev)));
}

std::optional<ShapeNode*> ShapeNode::NextNode() {
  auto last = Parent->Children.end();
  auto next = InParentIter;
  next++;
  if (next == last) {
    return std::nullopt;
  }
  return std::make_optional(&(*(next)));
}

void ShapeNode::AfterAddChild(std::list<ShapeNode>::iterator& childIter) {
  auto&& child = *childIter;
  child.Parent = this;
  child.InParentIter = childIter;
  // 子节点持有父节点地址, 因此父节点被移动走之后, 要更新所有子节点保存的地址
  for (auto&& i : child.Children) {
    i.Parent = &child;
  }
}

DefaultMessageBox::DefaultMessageBox(Application* app, const std::string& msg) : GuiObject(app, -999, true, "default_msg_box"), Message(msg) {}

void DefaultMessageBox::OnStart() {
  ImGui::OpenPopup(_app->I18n("error"));
}

void DefaultMessageBox::OnGui() {
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(_app->I18n("error"), nullptr)) {
    auto format = fmt::format(_app->I18n("asset_panel.load.reason"), Message.c_str());
    ImGui::Text("%s", format.c_str());
    if (OnOk != nullptr) {
      if (ImGui::Button(_app->I18n("ok"))) {
        _isAlive = false;
        OnOk(_app);
      }
      ImGui::SameLine();
    }
    if (ImGui::Button(_app->I18n("close"))) {
      _isAlive = false;
      if (OnCancel != nullptr) {
        OnCancel(_app);
      }
    }
    ImGui::EndPopup();
  }
}

LateUpdateCallback::LateUpdateCallback(int id, std::function<void(Application*)>&& callback) : Id(id), Callback(callback) {}

Application::Application(int argc, char** argv)
    : _logger(Logger::GetCategory("app")),
      _window(nullptr),
      _imRender({}),
      _i18n(),
      _factories(std::make_unique<FactoryManager>()),
      _hasWorkspace(false),
      _workRoot(std::filesystem::current_path()),
      _backgroundColor(0.2f, 0.3f, 0.3f) {
  LoadI18n(std::filesystem::current_path() / "i18n" / "zh_cn_utf8.json");
  for (auto&& i : GetRadCoreAssetFactories()) {
    _factories->RegisterFactory(i());
  }
  for (auto&& i : GetRadOfflineFactories()) {
    _factories->RegisterFactory(i());
  }
}

void Application::Run() {
  Start();
  Update();
  Destroy();
}

void Application::LoadI18n(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    _logger->warn("cannot load i18n file: {}", path.generic_string());
    return;
  }
  _i18n.clear();
  std::ifstream cfgStream(path);
  nlohmann::json cfg = nlohmann::json::parse(cfgStream);
  for (auto iter = cfg.begin(); iter != cfg.end(); iter++) {
    _i18n.emplace(iter.key(), iter->get<std::string>());
  }
}

const char* Application::I18n(const std::string& key) const {
  auto iter = _i18n.find(key);
  return iter == _i18n.end() ? key.c_str() : iter->second.c_str();
}

void Application::AddGui(Unique<GuiObject> ui) {
  _firstUseGui.emplace_back(std::move(ui));
}

void Application::NewScene(const std::filesystem::path& sceneFile) {
  CreateWorkspace(sceneFile, false);
}

void Application::OpenScene(const std::filesystem::path& sceneFile) {
  CreateWorkspace(sceneFile, true);
}

void Application::SaveScene() {
  // TODO:
  _logger->info("app::save scene");
}

void Application::CloseScene() {
  if (_hasWorkspace) {
    auto msgbox = std::make_unique<DefaultMessageBox>(this, "Save now scene?");
    msgbox->OnOk = [](Application* app2) {
      app2->SaveScene();
      app2->AddLateUpdate({0, [](Application* app3) { app3->ClearWorkspace(); }});
    };
    AddGui(std::move(msgbox));
  } else {
    AddLateUpdate({0, [](Application* app) { app->ClearWorkspace(); }});
  }
}

std::optional<GuiObject*> Application::FindUi(const std::string& name) {
  for (auto&& i : _firstUseGui) {
    if (i != nullptr && i->GetName() == name) {
      return std::make_optional(i.get());
    }
  }
  for (auto&& i : _activeGui) {
    if (i != nullptr && i->GetName() == name) {
      return std::make_optional(i.get());
    }
  }
  for (auto&& i : _iterGuiCache) {
    if (i != nullptr && i->GetName() == name) {
      return std::make_optional(i.get());
    }
  }
  return std::nullopt;
}

std::pair<bool, std::string> Application::LoadAsset(const std::string& name, const std::filesystem::path& loaction, int type) {
  if (_nameToAsset.find(name) != _nameToAsset.end()) {
    return {false, fmt::format("duplicate name: {}", name)};
  }
  if (!std::filesystem::exists(GetWorkRoot() / loaction)) {
    return {false, fmt::format("cannot find path: {}", loaction.string())};
  }
  std::unique_ptr<EditorAsset> ins;
  switch (type) {
    case 0: {
      ins = std::make_unique<ObjMeshAsset>();
      break;
    }
    default:
      return {false, fmt::format("unknown asset type: {}", type)};
  }
  ins->Name = name;
  ins->Loaction = loaction;
  auto loadResult = ins->Load(this);
  if (loadResult.first) {
    _nameToAsset.emplace(name, std::move(ins));
  }
  return loadResult;
}

ShapeNode Application::NewNode() {
  ShapeNode node{};
  node.Id = _nodeIdPool++;
  node.App = this;
  return node;
}

void Application::AddLateUpdate(const LateUpdateCallback& cb) {
  _lateUpdate.emplace_back(cb);
}

void Application::ChangePreviewResolution() {
  AddLateUpdate(
      {0, [](Application* app) {
         app->InitPreviewFrameBuffer(app->_camera.Resolution.x(), app->_camera.Resolution.y());
       }});
}

void Application::Start() {
  InitGraphics();
  InitImGui();
  InitBuiltinShapes();
  InitBasicGuiObject();
}

void Application::Update() {
  while (!glfwWindowShouldClose(_window)) {
    CollectRenderItem();
    UpdateImGui();
    DrawStartPass();
    DrawItemPass();
    DrawImGuiPass();
    OnLateUpdate();
    glfwSwapBuffers(_window);
    glfwPollEvents();
    std::this_thread::yield();
  }
}

void Application::Destroy() {
  glfwDestroyWindow(_window);
  _window = nullptr;
  glfwTerminate();
}

void Application::InitGraphics() {
  glfwSetErrorCallback([](int error, const char* description) {
    Logger::Get()->error("glfw err[{}]: {}", error, description);
  });
  if (!glfwInit()) {
    throw RadInvalidOperationException("cannot init glfw");
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(NDEBUG)
  glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
#else
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
  GLFWwindow* window = glfwCreateWindow(1440, 900, "Rad Editor", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw RadInvalidOperationException("cannot create window");
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  int version = gladLoadGL(glfwGetProcAddress);
  _logger->info("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  auto driverInfo = glGetString(GL_VERSION);
  std::string driver((char*)driverInfo);
  auto deviceName = glGetString(GL_RENDERER);
  std::string device((char*)deviceName);
  _logger->info("Device: {}", device);
  _logger->info("Driver: {}", driver);
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(
      [](GLenum source,
         GLenum type,
         GLuint id,
         GLenum severity,
         GLsizei length,
         GLchar const* message,
         void const* userParam) { static_cast<const Application*>(userParam)->GLDebugMessage(source, type, id, severity, length, message); },
      this);
  _window = window;
}

void Application::InitImGui() {
  GLuint vao;
  glCreateVertexArrays(1, &vao);
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);
  GLuint ibo;
  glCreateBuffers(1, &ibo);
  glNamedBufferStorage(ibo, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);
  glVertexArrayElementBuffer(vao, ibo);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(ImDrawVert));
  glEnableVertexArrayAttrib(vao, 0);
  glEnableVertexArrayAttrib(vao, 1);
  glEnableVertexArrayAttrib(vao, 2);
  glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, pos));
  glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, uv));
  glVertexArrayAttribFormat(vao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, IM_OFFSETOF(ImDrawVert, col));
  glVertexArrayAttribBinding(vao, 0, 0);
  glVertexArrayAttribBinding(vao, 1, 0);
  glVertexArrayAttribBinding(vao, 2, 0);
  GLuint imShader[2];
  bool isVsPass = GLCompileShader(imguiVertShader, GL_VERTEX_SHADER, &imShader[0]);
  bool isFsPass = GLCompileShader(imguiFragShader, GL_FRAGMENT_SHADER, &imShader[1]);
  if (!isVsPass || !isFsPass) {
    throw RadInvalidOperationException("cannot compile imgui shaders");
  }
  GLuint prog;
  bool isLink = GLLinkProgram(imShader, 2, &prog);
  if (!isLink) {
    throw RadInvalidOperationException("cannot linke imgui program");
  }
  GLuint ubo;
  glCreateBuffers(1, &ubo);
  glNamedBufferStorage(ubo, sizeof(Matrix4f), nullptr, GL_DYNAMIC_STORAGE_BIT);
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  ImFontConfig cfg = ImFontConfig();
  cfg.FontDataOwnedByAtlas = false;
  cfg.RasterizerMultiply = 1.5f;
  cfg.SizePixels = 720.0f / 32.0f;
  cfg.PixelSnapH = true;
  cfg.OversampleH = 4;
  cfg.OversampleV = 4;
  ImFont* font = io.Fonts->AddFontFromFileTTF(
      "fonts/SourceHanSerifCN-Medium.ttf",
      cfg.SizePixels,
      &cfg,
      io.Fonts->GetGlyphRangesChineseFull());
  unsigned char* pixels = nullptr;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  GLuint fontTexture;
  glCreateTextures(GL_TEXTURE_2D, 1, &fontTexture);
  glTextureParameteri(fontTexture, GL_TEXTURE_MAX_LEVEL, 0);
  glTextureParameteri(fontTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(fontTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureStorage2D(fontTexture, 1, GL_RGBA8, width, height);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTextureSubImage2D(fontTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  io.Fonts->TexID = (ImTextureID)(intptr_t)fontTexture;
  io.FontDefault = font;
  io.DisplayFramebufferScale = ImVec2(1, 1);
  _imRender = {vao, vbo, ibo, ubo, prog, fontTexture};

  ImGui_ImplGlfw_InitForOpenGL(_window, true);
}

void Application::InitPreviewFrameBuffer(int width, int height) {
  auto crtfbo = [&]() {
    glCreateFramebuffers(1, &_prevFbo.Fbo);
    glCreateTextures(GL_TEXTURE_2D, 1, &_prevFbo.ColorTex);
    glTextureParameteri(_prevFbo.ColorTex, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(_prevFbo.ColorTex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(_prevFbo.ColorTex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // int width, height;
    // glfwGetFramebufferSize(_window, &width, &height);
    glTextureStorage2D(_prevFbo.ColorTex, 1, GL_RGBA8, width, height);
    glNamedFramebufferTexture(_prevFbo.Fbo, GL_COLOR_ATTACHMENT0, _prevFbo.ColorTex, 0);
    glCreateRenderbuffers(1, &_prevFbo.Rbo);
    glNamedRenderbufferStorage(_prevFbo.Rbo, GL_DEPTH_COMPONENT, width, height);
    glNamedFramebufferRenderbuffer(_prevFbo.Fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _prevFbo.Rbo);
    auto status = glCheckNamedFramebufferStatus(_prevFbo.Fbo, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      _logger->error("cannot create preview fbo, {}", status);
      int colName;
      glGetNamedFramebufferAttachmentParameteriv(_prevFbo.Fbo, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &colName);
      int depName;
      glGetNamedFramebufferAttachmentParameteriv(_prevFbo.Fbo, GL_DEPTH_COMPONENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depName);
      _logger->error("attached color0: {}, depth: {}", colName, depName);
    }
    _prevFbo.Width = width;
    _prevFbo.Height = height;
  };

  if (_prevFbo.Fbo == 0) {
    crtfbo();

    GLuint shaders[2];
    bool isVsPass = GLCompileShader(normalVert, GL_VERTEX_SHADER, &shaders[0]);
    bool isFsPass = GLCompileShader(normalFrag, GL_FRAGMENT_SHADER, &shaders[1]);
    if (!isVsPass || !isFsPass) {
      throw RadInvalidOperationException("cannot compile preview shaders");
    }
    GLuint prog;
    bool isLink = GLLinkProgram(shaders, 2, &prog);
    if (!isLink) {
      throw RadInvalidOperationException("cannot linke preview program");
    }
    _prevFbo.ShaderProgram = prog;
    GLuint ubo;
    glCreateBuffers(1, &ubo);
    glNamedBufferStorage(ubo, sizeof(PerFrameData), nullptr, GL_DYNAMIC_STORAGE_BIT);
    _prevFbo.UboPerFrame = ubo;
    _prevFbo.PerFrame = {};
  } else {
    glDeleteFramebuffers(1, &_prevFbo.Fbo);
    glDeleteTextures(1, &_prevFbo.ColorTex);
    glDeleteRenderbuffers(1, &_prevFbo.Rbo);
    crtfbo();
  }
}

void Application::InitBuiltinShapes() {
  {
    auto sphere = CreateSphere(1, 32);
    auto& s = _builtinShape.Sphere;
    std::tie(s.VAO, s.VBO, s.IBO, s.Indices) = UploadToGpu(sphere);
    s.Name = "sphere";
  }
  {
    auto cube = CreateCube(1);
    auto& s = _builtinShape.Cube;
    std::tie(s.VAO, s.VBO, s.IBO, s.Indices) = UploadToGpu(cube);
    s.Name = "cube";
  }
  {
    auto rect = CreateRect(1);
    auto& s = _builtinShape.Rect;
    std::tie(s.VAO, s.VBO, s.IBO, s.Indices) = UploadToGpu(rect);
    s.Name = "rectangle";
  }
}

void Application::UpdateImGui() {
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  OnGui();
  ImGui::Render();
}

void Application::DrawStartPass() {
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  glViewport(0, 0, width, height);
  glClearColor(_backgroundColor.x(), _backgroundColor.y(), _backgroundColor.z(), 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Application::DrawItemPass() {
  if (_prevFbo.Fbo == 0) {
    return;
  }
  glUseProgram(_prevFbo.ShaderProgram);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, _prevFbo.UboPerFrame);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_SCISSOR_TEST);

  glBindFramebuffer(GL_FRAMEBUFFER, _prevFbo.Fbo);
  glViewport(0, 0, _prevFbo.Width, _prevFbo.Height);
  glClearColor(1 - _backgroundColor.x(), 1 - _backgroundColor.y(), 1 - _backgroundColor.z(), 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    // auto rot = _camera.Rotation.toRotationMatrix();
    // Vector3f front = (rot * Vector3f(0, 0, -1)).normalized();
    // Vector3f up = (rot * Vector3f(0, 1, 0)).normalized();
    // _camera.ToView = LookAt(_camera.Position, _camera.Position + front, up);

    _camera.ToView = LookAt(_camera.Position, _camera.Target, _camera.Up);
    _camera.ToPersp = Perspective(_camera.Fovy, (float)_prevFbo.Width / _prevFbo.Height, _camera.NearZ, _camera.FarZ);
  }

  auto& perFrame = _prevFbo.PerFrame;
  perFrame.View = _camera.ToView;
  perFrame.Persp = _camera.ToPersp;

  for (auto&& i : _renderItems) {
    GLuint vao;
    GLsizei indcnt;
    if (i->ShapeAsset.Ptr == nullptr) {
      vao = i->ShapeBuildin->VAO;
      indcnt = i->ShapeBuildin->Indices;
    } else {
      auto shape = static_cast<ObjMeshAsset*>(i->ShapeAsset.Ptr);
      vao = shape->VAO;
      indcnt = shape->Indices;
    }
    glBindVertexArray(vao);
    perFrame.Model = i->ToWorld;
    perFrame.InvModel = i->ToLocal;
    perFrame.MVP = perFrame.Persp * perFrame.View * perFrame.Model;
    glNamedBufferSubData(_prevFbo.UboPerFrame, 0, sizeof(PerFrameData), &perFrame);
    glDrawElementsBaseVertex(GL_TRIANGLES, indcnt, GL_UNSIGNED_INT, nullptr, 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::DrawImGuiPass() {
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  glViewport(0, 0, width, height);
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_SCISSOR_TEST);

  glUseProgram(_imRender.shaderProgram);
  glBindTextures(0, 1, &_imRender.fontTex);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, _imRender.uboPerFrame);

  glBindVertexArray(_imRender.vao);

  ImDrawData* drawData = ImGui::GetDrawData();
  {
    const float L = drawData->DisplayPos.x;
    const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float T = drawData->DisplayPos.y;
    const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    const float ortho[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f}};
    glNamedBufferSubData(_imRender.uboPerFrame, 0, sizeof(Matrix4f), ortho);
  }
  for (int n = 0; n < drawData->CmdListsCount; n++) {
    const ImDrawList* cmdList = drawData->CmdLists[n];
    glNamedBufferSubData(_imRender.vbo, 0, (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert), cmdList->VtxBuffer.Data);
    glNamedBufferSubData(_imRender.ibo, 0, (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), cmdList->IdxBuffer.Data);
    for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
      const ImVec4 cr = pcmd->ClipRect;
      glScissor((int)cr.x, (int)(height - cr.w), (int)(cr.z - cr.x), (int)(cr.w - cr.y));
      glBindTextureUnit(0, (GLuint)(intptr_t)pcmd->TextureId);
      glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
    }
  }
  glScissor(0, 0, width, height);
}

void Application::GLDebugMessage(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    GLchar const* message) const {
  auto src = [](GLenum src) {
    switch (src) {
      case GL_DEBUG_SOURCE_API:
        return "API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "Window System";
      case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "Shader Compiler";
      case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "Third Party";
      case GL_DEBUG_SOURCE_APPLICATION:
        return "App";
      case GL_DEBUG_SOURCE_OTHER:
        return "Other";
      default:
        return "Unknown Source";
    }
  };
  if (source == GL_DEBUG_SOURCE_APPLICATION && severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  const char* str = "[GL:{}] {}";
  switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      _logger->debug(str, src(source), message);
      break;
    case GL_DEBUG_SEVERITY_LOW:
      _logger->info(str, src(source), message);
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      _logger->warn(str, src(source), message);
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      _logger->error(str, src(source), message);
      break;
  }
}

bool Application::GLCompileShader(const char* source, GLenum type, GLuint* shader) {
  GLuint glShader = glCreateShader(type);
  glShaderSource(glShader, 1, &source, nullptr);
  glCompileShader(glShader);
  int success;
  glGetShaderiv(glShader, GL_COMPILE_STATUS, &success);
  if (success) {
    *shader = glShader;
  } else {
    int length;
    glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &length);
    std::string info(length + 1, '\0');
    glGetShaderInfoLog(glShader, length, nullptr, info.data());
    _logger->error("compile shader error\n{}", info);
    glDeleteShader(glShader);
  }
  return success;
}

bool Application::GLLinkProgram(const GLuint* shader, int count, GLuint* program) {
  GLuint glProgram = glCreateProgram();
  for (int i = 0; i < count; i++) {
    glAttachShader(glProgram, shader[i]);
  }
  glLinkProgram(glProgram);
  int success;
  glGetProgramiv(glProgram, GL_LINK_STATUS, &success);
  if (success) {
    *program = glProgram;
  } else {
    int length;
    glGetProgramiv(glProgram, GL_INFO_LOG_LENGTH, &length);
    std::string info(length + 1, '\0');
    glGetProgramInfoLog(glProgram, length, nullptr, info.data());
    _logger->error("link program error\n{}", info);
    glDeleteProgram(glProgram);
  }
  return success;
}

void Application::InitBasicGuiObject() {
  AddGui(std::make_unique<GuiMainBar>(this));
}

void Application::OnGui() {
  auto priorityCompare = [](const auto& l, const auto& r) {
    return l->GetPriority() > r->GetPriority();
  };
  _iterGuiCache.clear();
  _activeGui.swap(_iterGuiCache);
  std::sort(_firstUseGui.begin(), _firstUseGui.end(), priorityCompare);
  for (auto&& i : _firstUseGui) {
    i->OnStart();
    if (i->IsAlive()) {
      _iterGuiCache.emplace_back(std::move(i));
    }
  }
  _firstUseGui.clear();
  std::sort(_iterGuiCache.begin(), _iterGuiCache.end(), priorityCompare);
  for (auto&& i : _iterGuiCache) {
    if (i->IsAlive()) {
      i->OnGui();
      _activeGui.emplace_back(std::move(i));
    }
  }
  ImGui::ShowDemoWindow();
}

void Application::OnLateUpdate() {
  for (auto& i : _lateUpdate) {
    i.Callback(this);
  }
  _lateUpdate.clear();
}

void Application::CollectRenderItem() {
  _renderItems.clear();
  if (!_hasWorkspace) {
    return;
  }
  _recTemp.emplace(_root.get());
  while (!_recTemp.empty()) {
    auto node = _recTemp.front();
    _recTemp.pop();
    if (node->Parent != nullptr) {
      {
        Eigen::Translation<Float, 3> t(node->Position);
        Eigen::DiagonalMatrix<Float, 3> s(node->Scale);
        auto r = node->Rotation.toRotationMatrix();
        Eigen::Transform<Float, 3, Eigen::Affine> affine = t * r * s;
        node->ToWorld = affine.matrix();
      }
      node->ToWorld = node->ToWorld * node->Parent->ToWorld;
      node->ToLocal = node->ToWorld.inverse();
    }
    if (node->ShapeAsset.Ptr != nullptr || node->ShapeBuildin != nullptr) {
      _renderItems.emplace_back(node);
    }
    for (auto&& i : node->Children) {
      _recTemp.emplace(&i);
    }
  }
}

void Application::BuildScene(const nlohmann::json& cfg) {
  if (cfg.contains("assets")) {
    const auto& assets = cfg["assets"];
    for (const auto& assetNode : assets) {
      std::string type = assetNode["type"];
      std::string name = assetNode["name"];
      std::string location = assetNode["location"];
      std::filesystem::path pathLocal(location);
      _logger->debug("load asset: {}, {}, {}", type, name, location);
      if (!std::filesystem::exists(pathLocal)) {
        pathLocal = _workRoot / pathLocal;
        if (!std::filesystem::exists(pathLocal)) {
          AddGui(std::make_unique<DefaultMessageBox>(this, fmt::format("cannot load file {}", location)));
          return;
        }
      }
      if (type == "model_obj") {
        LoadAsset(name, pathLocal, 0);
      }
    }
  }
  if (cfg.contains("scene")) {
    auto&& scene = cfg["scene"];
    std::queue<std::pair<const nlohmann::json*, ShapeNode*>> q;
    for (auto& i : scene) {
      q.emplace(std::make_pair(&i, _root.get()));
    }
    while (!q.empty()) {
      auto [data, parent] = q.front();
      q.pop();
      auto&& i = *data;
      auto&& node = parent->AddChildLast(NewNode());
      node.Config = i;
      if (i.contains("__name")) {
        node.Name = i["__name"];
      } else {
        node.Name = fmt::format("node {}", node.Id);
      }
      if (i.contains("to_world")) {
        auto toWorld = i["to_world"];
        if (toWorld.is_array()) {
          auto mat = GetMat4(toWorld);
          std::tie(node.Position, node.Rotation, node.Scale) = DecomposeTransform(mat);
        } else if (toWorld.is_object()) {
          if (toWorld.contains("position")) {
            node.Position = GetVec3(toWorld["position"]);
          }
          if (toWorld.contains("scale")) {
            node.Scale = GetVec3(toWorld["scale"]);
          }
          if (toWorld.contains("rotate")) {
            auto& rotateNode = toWorld["rotate"];
            Vector3f axis{0, 1, 0};
            float angle{0};
            if (rotateNode.contains("axis")) {
              axis = GetVec3(rotateNode["axis"]);
            }
            if (rotateNode.contains("angle")) {
              angle = rotateNode["angle"];
            }
            Eigen::AngleAxisf aa(Math::Radian(angle), axis);
            node.Rotation = Eigen::Quaternionf(aa);
          }
        }
      }
      if (i.contains("shape")) {
        const auto& shape = i["shape"];
        std::string type = shape["type"];
        if (type == "sphere") {
          node.ShapeBuildin = &_builtinShape.Sphere;
          if (shape.contains("center")) {
            Vector3f center{0, 0, 0};
            if (shape.contains("center")) {
              const auto& cnode = shape["center"];
              center[0] = cnode[0];
              center[1] = cnode[1];
              center[2] = cnode[2];
            }
            float radius{1};
            if (shape.contains("radius")) {
              radius = shape["radius"];
            }
            node.Position += center;
            node.Scale = Vector3f::Constant(radius);
          }
        } else if (type == "cube") {
          node.ShapeBuildin = &_builtinShape.Cube;
        } else if (type == "rectangle") {
          node.ShapeBuildin = &_builtinShape.Rect;
        } else {
          std::string assetName = shape["asset_name"];
          node.ShapeAsset = EditorAssetGuard(GetAssets().at(assetName).get());
        }
      }
      if (i.contains("children")) {
        auto children = i["children"];
        for (auto& child : children) {
          q.emplace(std::make_pair(&child, &node));
        }
      }
    }
  }
  if (cfg.contains("camera")) {
    auto cameraNode = cfg["camera"];
    Vector3f origin{0, 0, -1};
    Vector3f target{0, 0, 0};
    Vector3f up{0, 1, 0};
    if (cameraNode.contains("origin")) {
      if (cameraNode.contains("origin")) {
        origin = GetVec3(cameraNode["origin"]);
      }
      if (cameraNode.contains("target")) {
        target = GetVec3(cameraNode["target"]);
      }
      if (cameraNode.contains("up")) {
        up = GetVec3(cameraNode["up"]);
      }
    } else {
      Matrix4f mat = GetMat4(cameraNode["to_world"]);
      Eigen::Affine3f aff(mat);
      origin = aff.translation();
      target = (aff.linear() * Vector3f(0, 0, 1)).normalized() + origin;
      up = (aff.linear() * Vector3f(0, 1, 0)).normalized();
    }
    _camera.Position = origin;
    _camera.Target = target;
    _camera.Up = up;
    if (cameraNode.contains("near")) {
      _camera.NearZ = cameraNode["near"];
    }
    if (cameraNode.contains("far")) {
      _camera.FarZ = cameraNode["far"];
    }
    if (cameraNode.contains("fov")) {
      _camera.Fovy = cameraNode["fov"];
    }
    if (cameraNode.contains("resolution")) {
      auto v = GetVec2(cameraNode["resolution"]).cast<int>();
      _camera.Resolution = v;
    }
  }
  if (cfg.contains("renderer")) {
    auto rendererNode = cfg["renderer"];
    std::string type = rendererNode["type"];
  }
  _workConfig = cfg;
}

void Application::CreateWorkspace(const std::filesystem::path& sceneFile, bool isBuild) {
  auto crtWs = [=](Application* app3) {
    app3->ClearWorkspace();
    app3->_hasWorkspace = true;
    auto workDir = sceneFile.parent_path();
    app3->_workRoot = workDir;
    app3->_sceneName = sceneFile.filename().replace_extension("").string();

    app3->AddGui(std::make_unique<GuiAssetPanel>(app3));
    app3->AddGui(std::make_unique<GuiHierarchy>(app3));
    app3->AddGui(std::make_unique<GuiCamera>(app3));
    app3->AddGui(std::make_unique<GuiPreviewScene>(app3));
    app3->AddGui(std::make_unique<GuiOfflineRenderPanel>(app3));

    app3->_root = std::make_unique<ShapeNode>();
    app3->_root->Name = app3->_sceneName;
    app3->_nodeIdPool = 1;
    if (isBuild) {
      nlohmann::json cfg;
      {
        std::ifstream cfgStream(sceneFile);
        if (!cfgStream.is_open()) {
          app3->AddGui(std::make_unique<DefaultMessageBox>(app3, fmt::format("cannot open file {}", sceneFile.string())));
          return;
        }
        cfg = nlohmann::json::parse(cfgStream);
      }
      app3->BuildScene(cfg);
      app3->InitPreviewFrameBuffer(_camera.Resolution.x(), _camera.Resolution.y());
    }
  };

  if (_hasWorkspace) {
    auto msgbox = std::make_unique<DefaultMessageBox>(this, "Save now scene?");
    msgbox->OnOk = [sceneFile, crtWs](Application* app2) {
      app2->SaveScene();
      app2->_hasWorkspace = false;
      app2->AddLateUpdate({1, crtWs});
    };
    AddGui(std::move(msgbox));
  } else {
    AddLateUpdate({0, crtWs});
  }
}

void Application::ClearWorkspace() {
  _hasWorkspace = {false};

  _hasWorkspace = false;
  _iterGuiCache.clear();
  _activeGui.clear();
  _firstUseGui.clear();
  _nameToAsset.clear();
  _root = nullptr;
  InitBasicGuiObject();
}

TriangleModel CreateSphere(float radius, int numberSlices) {
  const Vector3f axisX = {1.0f, 0.0f, 0.0f};

  uint32_t numberParallels = numberSlices / 2;
  uint32_t numberVertices = (numberParallels + 1) * (numberSlices + 1);
  uint32_t numberIndices = numberParallels * numberSlices * 6;

  float angleStep = (2.0f * PI) / ((float)numberSlices);

  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});

  for (uint32_t i = 0; i < numberParallels + 1; i++) {
    for (uint32_t j = 0; j < (uint32_t)(numberSlices + 1); j++) {
      uint32_t vertexIndex = (i * (numberSlices + 1) + j);
      uint32_t normalIndex = (i * (numberSlices + 1) + j);
      uint32_t texCoordsIndex = (i * (numberSlices + 1) + j);

      float px = radius * std::sin(angleStep * (float)i) * std::sin(angleStep * (float)j);
      float py = radius * std::cos(angleStep * (float)i);
      float pz = radius * std::sin(angleStep * (float)i) * std::cos(angleStep * (float)j);
      vertices[vertexIndex] = {px, py, pz};

      float nx = vertices[vertexIndex].x() / radius;
      float ny = vertices[vertexIndex].y() / radius;
      float nz = vertices[vertexIndex].z() / radius;
      normals[normalIndex] = {nx, ny, nz};

      float tx = (float)j / (float)numberSlices;
      float ty = 1.0f - (float)i / (float)numberParallels;
      texCoords[texCoordsIndex] = {tx, ty};
    }
  }

  uint32_t indexIndices = 0;
  std::vector<uint32_t> indices(numberIndices, uint32_t{});
  for (uint32_t i = 0; i < numberParallels; i++) {
    for (uint32_t j = 0; j < (uint32_t)(numberSlices); j++) {
      indices[indexIndices++] = i * ((uint32_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((uint32_t)i + 1) * ((uint32_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((uint32_t)i + 1) * ((uint32_t)numberSlices + 1) + ((uint32_t)j + 1);

      indices[indexIndices++] = i * ((uint32_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((uint32_t)i + 1) * ((uint32_t)numberSlices + 1) + ((uint32_t)j + 1);
      indices[indexIndices++] = (uint32_t)i * ((uint32_t)numberSlices + 1) + ((uint32_t)j + 1);
    }
  }
  return TriangleModel(vertices.data(), numberVertices, indices.data(), numberIndices, normals.data(), texCoords.data());
}

TriangleModel CreateCube(float halfExtend) {
  const float cubeVertices[] =
      {-1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f,
       +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f};
  const float cubeNormals[] =
      {0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
       0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f,
       0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
       0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
       -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f};
  const float cubeTexCoords[] =
      {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
       1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  const uint32_t cubeIndices[] =
      {0, 2, 1, 0, 3, 2,
       4, 5, 6, 4, 6, 7,
       8, 9, 10, 8, 10, 11,
       12, 15, 14, 12, 14, 13,
       16, 17, 18, 16, 18, 19,
       20, 23, 22, 20, 22, 21};

  constexpr const uint32_t numberVertices = 24;
  constexpr const uint32_t numberIndices = 36;

  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  for (uint32_t i = 0; i < numberVertices; i++) {
    vertices[i] = {cubeVertices[i * 4 + 0] * halfExtend,
                   cubeVertices[i * 4 + 1] * halfExtend,
                   cubeVertices[i * 4 + 2] * halfExtend};
    normals[i] = {cubeNormals[i * 3 + 0],
                  cubeNormals[i * 3 + 1],
                  cubeNormals[i * 3 + 2]};
    texCoords[i] = {cubeTexCoords[i * 2 + 0],
                    cubeTexCoords[i * 2 + 1]};
  }
  return TriangleModel(vertices.data(), numberVertices, cubeIndices, numberIndices, normals.data(), texCoords.data());
}

TriangleModel CreateRect(float halfExtend) {
  const float quadVertices[] =
      {-1.0f, 0.0f, -1.0f, +1.0f,
       +1.0f, 0.f, -1.0f, +1.0f,
       -1.0f, 0.f, +1.0f, +1.0f,
       +1.0f, 0.f, +1.0f, +1.0f};
  const float quadNormal[] =
      {0.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f};
  const float quadTex[] =
      {0.0f, 0.0f,
       1.0f, 0.0f,
       0.0f, 1.0f,
       1.0f, 1.0f};
  const uint32_t quadIndices[] =
      {0, 2, 1,
       1, 2, 3};
  constexpr const uint32_t numberVertices = 4;
  constexpr const uint32_t numberIndices = 6;
  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  for (uint32_t i = 0; i < numberVertices; i++) {
    vertices[i] = {quadVertices[i * 4 + 0] * halfExtend,
                   quadVertices[i * 4 + 1] * halfExtend,
                   quadVertices[i * 4 + 2]};
    normals[i] = {quadNormal[i * 3 + 0],
                  quadNormal[i * 3 + 1],
                  quadNormal[i * 3 + 2]};
    texCoords[i] = {quadTex[i * 2 + 0],
                    quadTex[i * 2 + 1]};
  }
  return TriangleModel(vertices.data(), numberVertices, quadIndices, numberIndices, normals.data(), texCoords.data());
}

}  // namespace Rad
