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

namespace Rad {

constexpr const float PI = 3.1415926f;

const char* imguiVertShader = R"(
#version 450 core
layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;
layout (std140, binding = 0) uniform PerFrameData {
  uniform mat4 MVP;
};
out vec2 Frag_UV;
out vec4 Frag_Color;
void main() {
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = MVP * vec4(Position.xy,0,1);
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

Matrix4f LookAt(const Vector3f& eye, const Vector3f& target, const Vector3f& up) {
  Vector3f f = (target - eye).normalized();
  Vector3f u = up.normalized();
  Vector3f s = f.cross(u).normalized();
  u = s.cross(f);
  Matrix4f mat = Matrix4f::Zero();
  mat(0, 0) = s.x();
  mat(0, 1) = s.y();
  mat(0, 2) = s.z();
  mat(0, 3) = -s.dot(eye);
  mat(1, 0) = u.x();
  mat(1, 1) = u.y();
  mat(1, 2) = u.z();
  mat(1, 3) = -u.dot(eye);
  mat(2, 0) = -f.x();
  mat(2, 1) = -f.y();
  mat(2, 2) = -f.z();
  mat(2, 3) = f.dot(eye);
  mat.row(3) << 0, 0, 0, 1;
  return mat;
}

Matrix4f Perspective(float fovy, float aspect, float zNear, float zFar) {
  Matrix4f tr = Matrix4f::Zero();
  float radf = PI * fovy / 180.0f;
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

EditorAssetGuard::~EditorAssetGuard() noexcept {
  if (Ptr != nullptr) {
    Ptr->Reference--;
    Ptr = nullptr;
  }
}

std::pair<bool, std::string> ObjMeshAsset::Load(Application* app) {
  WavefrontObjReader reader(std::make_unique<std::ifstream>(app->GetWorkRoot() / Loaction));
  reader.Read();
  std::pair<bool, std::string> result;
  if (reader.HasError()) {
    result.first = false;
    result.second = reader.Error();
  } else {
    auto model = reader.ToModel();
    std::vector<GLVertex> vertices;
    vertices.reserve(model.VertexCount());
    for (size_t i = 0; i < model.VertexCount(); i++) {
      vertices.emplace_back(GLVertex{model.GetPosition()[i], model.GetNormal()[i], model.GetUV()[i]});
    }
    auto indices = model.GetIndices();
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
    VAO = vao;
    VBO = vbo;
    IBO = ibo;
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
}

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
  _hasRequestNewScene = true;
  _requestNewScenePath = sceneFile;
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
  return node;
}

void Application::Start() {
  InitGraphics();
  InitImGui();
  InitBasicGuiObject();
}

void Application::Update() {
  while (!glfwWindowShouldClose(_window)) {
    CollectRenderItem();
    UpdateImGui();
    DrawStartPass();
    DrawImGuiPass();
    ExecuteRequestNewScene();
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
      io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
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

void Application::DrawImGuiPass() {
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
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

void Application::ExecuteRequestNewScene() {
  if (!_hasRequestNewScene) {
    return;
  }
  _hasRequestNewScene = false;

  if (_hasWorkspace) {
    // TODO: 提示储存场景
  }
  _hasWorkspace = true;
  auto workDir = _requestNewScenePath.parent_path();
  _workRoot = workDir;
  _sceneName = _requestNewScenePath.filename().replace_extension("").string();

  _iterGuiCache.clear();
  _activeGui.clear();
  _firstUseGui.clear();
  InitBasicGuiObject();
  AddGui(std::make_unique<GuiAssetPanel>(this));
  AddGui(std::make_unique<GuiHierarchy>(this));
  AddGui(std::make_unique<GuiCamera>(this));

  _root = std::make_unique<ShapeNode>();
  _root->Name = _sceneName;
  _nodeIdPool = 1;
}

void Application::CollectRenderItem() {
  if (!_hasWorkspace) {
    return;
  }
  _recTemp.emplace(_root.get());
  while (!_recTemp.empty()) {
    auto node = _recTemp.front();
    _recTemp.pop();
    if (node->Parent != nullptr) {
      node->ToWorld = node->ToWorld * node->Parent->ToWorld;
    }
    if (node->ShapeAsset.Ptr != nullptr) {
      _renderItems.emplace_back(node);
    }
    for (auto&& i : node->Children) {
      _recTemp.emplace(&i);
    }
  }
  // Vector3f front = (_camera.Rotation * Vector3f(0, 0, 1)).normalized();
  // Vector3f up = (_camera.Rotation * Vector3f(0, 1, 0)).normalized();
  // _camera.ToView = LookAt(_camera.Position, _camera.Position + front, up);
  // _camera.ToPersp = Perspective(_camera.Fovy, 0, 0, 0);
}

}  // namespace Rad
