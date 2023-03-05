#include <rad/offline/editor/gui_object.h>

#include <imgui.h>
#include <random>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiObject::GuiObject(Application* app, int priority, bool isAlive, const std::string name)
    : _app(app), _priority(priority), _isAlive(isAlive), _name(name) {}

void GuiObject::DrawPSR(Vector3f* pos, Vector3f* sca, Eigen::Quaternionf* rot) {
  ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_NoRoundToFormat;
  std::uniform_real_distribution<float> uni(0.15f, 0.25f);
  ImGui::DragFloat3(
      _app->I18n("position"),
      (float*)pos,
      uni(_app->GetRng()),
      0, 0,
      "%.3f", sliderFlags);
  ImGui::DragFloat3(
      _app->I18n("scale"),
      (float*)sca,
      uni(_app->GetRng()),
      0, 0,
      "%.3f", sliderFlags);
  {
    Eigen::Matrix3f rotMat = rot->toRotationMatrix();
    Vector3f radian = rotMat.eulerAngles(2, 0, 1);
    Vector3f degree(Math::Degree(radian.x()), Math::Degree(radian.y()), Math::Degree(radian.z()));
    ImGui::DragFloat3(
        _app->I18n("rotation"),
        (float*)(&degree),
        1 + uni(_app->GetRng()),
        0, 0,
        "%.3f", sliderFlags);
    Eigen::AngleAxisf rollAngle(Math::Radian(degree.x()), Eigen::Vector3f::UnitZ());
    Eigen::AngleAxisf yawAngle(Math::Radian(degree.y()), Eigen::Vector3f::UnitX());
    Eigen::AngleAxisf pitchAngle(Math::Radian(degree.z()), Eigen::Vector3f::UnitY());
    *rot = rollAngle * yawAngle * pitchAngle;
    rot->normalize();
  }
}

}  // namespace Rad
