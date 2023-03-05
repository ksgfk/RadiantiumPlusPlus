#include <rad/offline/editor/gui_camera.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiCamera::GuiCamera(Application* app) : GuiObject(app, 0, true, "camera_panel") {}

void GuiCamera::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(1080, 100), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(360, 240), ImGuiCond_Once);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("camera.title"), &IsOpen, 0)) {
    auto& cam = _app->GetCamera();
    DrawPSR(&cam.Position, &cam.Scale, &cam.Rotation);
    std::uniform_real_distribution<float> dist(0.1f, 0.3f);
    ImGui::DragFloat(_app->I18n("camera.fovy"), &cam.Fovy, dist(_app->GetRng()), 0.0001f, 360.0f);
    constexpr float fmx = std::numeric_limits<float>().max();
    ImGui::DragFloat(_app->I18n("camera.near_z"), &cam.NearZ, dist(_app->GetRng()), 0.0001f, fmx);
    ImGui::DragFloat(_app->I18n("camera.far_z"), &cam.FarZ, dist(_app->GetRng()), cam.NearZ + 0.001f, fmx);
    if (cam.FarZ <= cam.NearZ) {
      cam.FarZ = cam.NearZ + 0.001f;
    }
  }
}

}  // namespace Rad
