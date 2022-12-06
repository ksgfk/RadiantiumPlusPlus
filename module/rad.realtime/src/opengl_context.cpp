#if defined(_MSC_VER)
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#ifdef __cplusplus
}
#endif
#endif

#include <rad/realtime/opengl_context.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <rad/core/logger.h>
#include <rad/realtime/window.h>

namespace Rad {

static Share<spdlog::logger> _logger{nullptr};
static bool _isInitOgl{false};
static ImGuiContext* _imguiContext{nullptr};

void RadInitContextOpenGL() {
  if (!RadIsWindowActiveGlfw()) {
    throw RadInvalidOperationException("should create a glfw window before init ogl ctx");
  }
  if (_isInitOgl) {
    throw RadInvalidOperationException("opengl context is active!");
  }
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    throw RadInvalidOperationException("cannot init opengl context");
  }
  _logger = Logger::GetCategory("OpenGL");
  _logger->info("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  auto driverInfo = glGetString(GL_VERSION);
  std::string driver((char*)driverInfo);
  auto deviceName = glGetString(GL_RENDERER);
  std::string device((char*)deviceName);
  _logger->info("Device: {}", device);
  _logger->info("Driver: {}", driver);
  _isInitOgl = true;
}

void RadShutdownContextOpenGL() {
  _logger = nullptr;
  _isInitOgl = false;
}

void RadInitImguiOpenGL(const ImguiInitOptions& opts) {
  if (_imguiContext != nullptr) {
    throw RadInvalidOperationException("imgui is active!");
  }
  IMGUI_CHECKVERSION();
  _imguiContext = ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF(opts.FontsPath.c_str(), opts.FontsSize, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
  io.Fonts->Build();
  ImGui::StyleColorsLight();
  ImGui::GetStyle().ScaleAllSizes(opts.DpiScale);
  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)RadWindowHandlerGlfw(), false);
  ImGui_ImplOpenGL3_Init("#version 450 core");
}

void RadShutdownImguiOpenGL() {
  if (_imguiContext != nullptr) {
    ImGui::DestroyContext(_imguiContext);
    _imguiContext = nullptr;
  }
}

void RadImguiNewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void RadImguiRender() {
  ImGui::Render();
  GLFWwindow* window = (GLFWwindow*)RadWindowHandlerGlfw();
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace Rad
