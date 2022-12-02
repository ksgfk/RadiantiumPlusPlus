#include "editor_application.h"

#include <rad/realtime/window.h>
#include <rad/realtime/input.h>
#include <rad/realtime/model.h>

#include <rad/realtime/opengl/render_context_ogl.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imfilebrowser.h>

#include <unordered_map>

using namespace Rad::OpenGL;

namespace Rad::Editor {

class Transform {
 public:
  Transform* Parent{nullptr};
  Vector3f Position = Vector3f::Zero();
  Vector3f Scale = Vector3f::Constant(1);
  Eigen::Quaternionf Rotation = Eigen::Quaternionf::Identity();
  Matrix4f ToWorld;
  Matrix4f ToLocal;
};

class MeshRenderer {
 public:
};

class OfflineData {
 public:
};

class SceneObject {
 public:
  void OnGui() {
  }

 public:
  std::string Name;
  Transform Trans;
  Unique<MeshRenderer> Renderer;
  Unique<OfflineData> Data;
};

enum class AssetType {
  Model,
  Image
};

class AssetEntry {
 public:
  virtual ~AssetEntry() noexcept = default;

  AssetType Type;
  std::string Name;
  UInt32 RefCount;
};

class AssetModel final : AssetEntry {
 public:
  ~AssetModel() noexcept override = default;

  Unique<ImmutableModel> Model;
};

class AssetCenter {
 public:
  std::unordered_map<std::string, Unique<AssetEntry>> Entries;
};

struct RenderPassPayload {
};

class RenderPass {
 public:
  virtual ~RenderPass() noexcept;

  virtual void OnInit(GLContext* gl) = 0;
  virtual void OnRender(GLContext* gl, const RenderPassPayload&) = 0;
};

class EditorApplicationImpl {
 public:
  EditorApplicationImpl(int argc, char** argv) {
    Rad::Window::Init();
    WindowOptions wo{Vector2i(1280, 720), "Radiantium Editor v0.0.1"};
    _window = Window::Create(wo);
    _input = std::make_unique<Input>();
    _window->AddScrollListener([&](auto& w, const auto& s) { _input->OnScroll(s); });
    RenderContextOptions rco{};
    rco.EnableDebug = true;
    _renderCtx = std::make_unique<RenderContextOpenGL>(*_window, rco);
    _asset = std::make_unique<AssetCenter>();
    {
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.Fonts->AddFontFromFileTTF("fonts/SourceHanSerifCN-Medium.ttf", 21, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
      io.Fonts->Build();
      ImGui::StyleColorsLight();
      ImGui::GetStyle().ScaleAllSizes(1.25f);
      ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)_window->GetHandler(), true);
      ImGui_ImplOpenGL3_Init("#version 450 core");
    }
    _isWelcomePanel = true;
    _fileDialog.SetTitle("选择文件");
  }
  ~EditorApplicationImpl() {
    _input = nullptr;
    _renderCtx = nullptr;
    _window->Destroy();
    Rad::Window::Shutdown();
  }

  void Run() {
    while (!_window->ShouldClose()) {
      _window->PollEvent();
      _input->Update(*_window);

      // TODO: 这里放渲染pass
      _renderCtx->GetGL()->ClearColor(0, 0.2f, 0.3f, 0.1f);
      _renderCtx->GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        OnGui();
        ImGui::Render();
        const auto& size = _window->GetSize();
        _renderCtx->GetGL()->Viewport(0, 0, size.x(), size.y());
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      }

      _renderCtx->SwapBuffers();
    }
  }

  void OnGui() {
    ImGui::ShowDemoWindow();
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("文件")) {
        if (ImGui::MenuItem("从文件打开工作区")) {
          _fileDialog.Open();
          if (_fileDialog.HasSelected()) {
            _fileDialog.ClearSelected();
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    _fileDialog.Display();

    if (_isWelcomePanel) {
      ImGuiWindowFlags flags = 0;
      if (ImGui::Begin("主页", nullptr, flags)) {
        ImGui::End();
      }
    } else {
    }
  }

 private:
  Unique<Window> _window;
  Unique<Input> _input;
  Unique<RenderContextOpenGL> _renderCtx;
  Unique<AssetCenter> _asset;
  ImGui::FileBrowser _fileDialog;
  bool _isWelcomePanel;
};

EditorApplication::EditorApplication(int argc, char** argv) {
  _impl = std::make_shared<EditorApplicationImpl>(argc, argv);
}

void EditorApplication::Run() {
  _impl->Run();
}

}  // namespace Rad::Editor
