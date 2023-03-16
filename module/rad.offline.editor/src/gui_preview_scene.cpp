#include <rad/offline/editor/gui_preview_scene.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiPreviewScene::GuiPreviewScene(Application* app)
    : GuiObject(app, 0, true, "preview_scene") {}

void GuiPreviewScene::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(355, 43), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(722, 488), ImGuiCond_FirstUseEver);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("preview_scene.title"), &IsOpen, 0)) {
    ImVec2 winSize = ImGui::GetContentRegionAvail();
    auto&& fb = _app->GetPreviewFb();
    int imgWidth, imgHeight;
    bool isWidth = winSize.x < winSize.y;
    float aspect = (float)fb.Width / fb.Height;
    if (isWidth) {
      imgWidth = (int)winSize.x;
      imgHeight = (int)((float)imgWidth / aspect);
    } else {
      imgHeight = (int)winSize.y;
      imgWidth = (int)((float)imgHeight * aspect);
    }
    if (imgWidth > winSize.x) {
      imgWidth = (int)winSize.x;
      imgHeight = (int)((float)imgWidth / aspect);
    }
    if (imgHeight > winSize.y) {
      imgHeight = (int)winSize.y;
      imgWidth = (int)((float)imgHeight * aspect);
    }
    auto tex = (size_t)fb.ColorTex;
    //这里要反转y轴, 不然图像倒过来的
    ImGui::Image(
        reinterpret_cast<void*>(tex),
        ImVec2((float)imgWidth, (float)imgHeight),
        ImVec2(0, 1), ImVec2(1, 0));
  }
}

}  // namespace Rad
