#include <rad/offline/editor/gui_asset_panel.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiAssetPanel::GuiAssetPanel(Application* app)
    : GuiObject(app, 1, true, "asset_panel"),
      IsOpen(true) {}

void GuiAssetPanel::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("asset_panel.title"), &IsOpen, flags)) {
  }
}

}  // namespace Rad
