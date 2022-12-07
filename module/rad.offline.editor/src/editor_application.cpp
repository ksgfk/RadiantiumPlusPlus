#include <rad/offline/editor/editor_application.h>

#include <rad/realtime/window.h>
#include <rad/realtime/opengl_context.h>

#include <thread>

namespace Rad {

EditorApplication::EditorApplication(int argc, char** argv) {
  RadInitGlfw();
  Rad::GlfwWindowOptions opts{};
  opts.Title = "Rad-Offline Editor v0.0.1";
  opts.EnableOpenGL = true;
  opts.EnableOpenGLDebugLayer = true;
  opts.IsMaximize = true;
  RadCreateWindowGlfw(opts);
  RadShowWindowGlfw();
  RadInitContextOpenGL();
  _ui = std::make_unique<UIManager>(this);
}

EditorApplication::~EditorApplication() noexcept {
  _ui = nullptr;
  RadShutdownContextOpenGL();
  RadDestroyWindowGlfw();
  RadShutdownGlfw();
}

void EditorApplication::Run() {
  while (!Rad::RadShouldCloseWindowGlfw()) {
    Rad::RadPollEventGlfw();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _ui->NewFrame();
    _ui->OnGui();
    _ui->Draw();
    Rad::RadSwapBuffersGlfw();
    std::this_thread::yield();
  }
}

}  // namespace Rad
