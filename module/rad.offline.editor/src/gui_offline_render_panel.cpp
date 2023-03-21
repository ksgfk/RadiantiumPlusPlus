#include <rad/offline/editor/gui_offline_render_panel.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiOfflineRenderPanel::GuiOfflineRenderPanel(Application* app) : GuiObject(app, 0, true, "offline_render") {}

void GuiOfflineRenderPanel::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(375, 65), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("offline_render.title"), &IsOpen, 0)) {
    auto&& workCfg = _app->GetWorkspaceConfig();
    int RenderAlgo;
    if (workCfg.contains("renderer")) {
      std::string type = workCfg["renderer"]["type"];
      if (type == "ao") {
        RenderAlgo = 0;
      } else if (type == "path") {
        RenderAlgo = 1;
      } else if (type == "") {
        RenderAlgo = 2;
      } else if (type == "bdpt") {
        RenderAlgo = 3;
      } else {
        RenderAlgo = -1;
      }
    } else {
      RenderAlgo = -1;
    }
    const char* prev = RenderAlgo == -1 ? "" : RendererNames()[RenderAlgo];
    if (ImGui::BeginCombo(_app->I18n("offline_render.algorithm"), prev)) {
      for (int i = 0; i < RendererNameCount(); i++) {
        bool isSelected = (i == RenderAlgo);
        if (ImGui::Selectable(RendererNames()[i], isSelected)) {
          RenderAlgo = i;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    int spp;
    if (workCfg.contains("sampler")) {
      spp = workCfg["sampler"]["sample_count"];
    } else {
      nlohmann::json t;
      t.push_back(nlohmann::json::object_t::value_type("type", "independent"));
      t.push_back(nlohmann::json::object_t::value_type("sample_count", 256));
      workCfg.push_back(nlohmann::json::object_t::value_type("sampler", t));
      spp = 256;
    }
    ImGui::InputInt(_app->I18n("offline_render.spp"), &spp);
    workCfg["sampler"]["sample_count"] = spp;
    if (IsRendering) {
    }
  }
}

}  // namespace Rad
