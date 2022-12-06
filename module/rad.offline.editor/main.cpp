#include <rad/core/common.h>
#include <rad/core/logger.h>
#include <rad/realtime/window.h>
#include <rad/realtime/opengl_context.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>

int main(int argc, char** argv) {
  Rad::RadCoreInit();
  Rad::RadInitGlfw();
  try {
    Rad::GlfwWindowOptions opts{};
    opts.Size = {1280, 720};
    opts.Title = "Rad-Offline editor v0.0.1";
    opts.EnableOpenGL = true;
    opts.EnableOpenGLDebugLayer = true;
    Rad::RadCreateWindowGlfw(opts);
    Rad::RadShowWindowGlfw();
    Rad::RadInitContextOpenGL();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("fonts/SourceHanSerifCN-Medium.ttf", 21, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    io.Fonts->Build();
    ImGui::StyleColorsLight();
    ImGui::GetStyle().ScaleAllSizes(1.25f);
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)Rad::RadWindowHandlerGlfw(), false);
    ImGui_ImplOpenGL3_Init("#version 450 core");

    bool isPerspective = true;
    float fov = 27.f;
    float viewWidth = 10.f;  // for orthographic
    float camYAngle = 165.f / 180.f * 3.14159f;
    float camXAngle = 32.f / 180.f * 3.14159f;
    float cameraProjection[16];

    while (!Rad::RadShouldCloseWindowGlfw()) {
      Rad::RadPollEventGlfw();
      glClearColor(0.1f, 0.3f, 0.2f, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::ShowDemoWindow();

      // ImGuizmo::BeginFrame();

      ImGui::Render();
      Rad::Vector2i size = Rad::RadGetFrameBufferSizeGlfw();
      glViewport(0, 0, size.x(), size.y());
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      Rad::RadSwapBuffersGlfw();
    }
    Rad::RadShutdownContextOpenGL();
    Rad::RadDestroyWindowGlfw();
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  Rad::RadShutdownGlfw();
  Rad::RadCoreShutdown();
  return 0;
}
