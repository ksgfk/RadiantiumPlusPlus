#include <rad/offline/editor/gui_camera.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiCamera::GuiCamera(Application* app) : GuiObject(app, 0, true, "camera_panel") {}

void GuiCamera::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(1080, 100), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(360, 300), ImGuiCond_FirstUseEver);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("camera.title"), &IsOpen, 0)) {
    auto& cam = _app->GetCamera();
    {
      ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_NoRoundToFormat;
      std::uniform_real_distribution<float> uni(0.15f, 0.25f);
      ImGui::DragFloat3(
          _app->I18n("position"),
          (float*)&cam.Position,
          uni(_app->GetRng()),
          0, 0,
          "%.3f", sliderFlags);
      ImGui::DragFloat3(
          _app->I18n("target"),
          (float*)&cam.Target,
          uni(_app->GetRng()),
          0, 0,
          "%.3f", sliderFlags);
      ImGui::DragFloat3(
          _app->I18n("up"),
          (float*)&cam.Up,
          uni(_app->GetRng()),
          0, 0,
          "%.3f", sliderFlags);
      // {
      //   Eigen::Matrix3f rotMat = cam.Rotation.toRotationMatrix();
      //   Vector3f radian = rotMat.eulerAngles(2, 0, 1);
      //   Vector3f degree(Math::Degree(radian.x()), Math::Degree(radian.y()), Math::Degree(radian.z()));
      //   ImGui::DragFloat3(
      //       _app->I18n("rotation"),
      //       (float*)(&degree),
      //       1 + uni(_app->GetRng()),
      //       0, 0,
      //       "%.3f", sliderFlags);
      //   Eigen::AngleAxisf rollAngle(Math::Radian(degree.x()), Eigen::Vector3f::UnitZ());
      //   Eigen::AngleAxisf yawAngle(Math::Radian(degree.y()), Eigen::Vector3f::UnitX());
      //   Eigen::AngleAxisf pitchAngle(Math::Radian(degree.z()), Eigen::Vector3f::UnitY());
      //   cam.Rotation = rollAngle * yawAngle * pitchAngle;
      //   cam.Rotation.normalize();
      // }
    }
    std::uniform_real_distribution<float> dist(0.1f, 0.3f);
    ImGui::DragFloat(_app->I18n("camera.fovy"), &cam.Fovy, dist(_app->GetRng()), 0.0001f, 360.0f);
    constexpr float fmx = std::numeric_limits<float>().max();
    ImGui::DragFloat(_app->I18n("camera.near_z"), &cam.NearZ, dist(_app->GetRng()), 0.0001f, fmx);
    ImGui::DragFloat(_app->I18n("camera.far_z"), &cam.FarZ, dist(_app->GetRng()), cam.NearZ + 0.001f, fmx);
    if (cam.FarZ <= cam.NearZ) {
      cam.FarZ = cam.NearZ + 0.001f;
    }
    ImGui::DragInt2(_app->I18n("camera.resolution"), cam.Resolution.data());
    if (ImGui::Button(_app->I18n("camera.resolution.apply"))) {
      _app->ChangePreviewResolution();
    }
  }
}

}  // namespace Rad
