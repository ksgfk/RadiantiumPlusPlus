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
    int renderAlgo;
    if (workCfg.contains("renderer")) {
      std::string type = workCfg["renderer"]["type"];
      if (type == "ao") {
        renderAlgo = 0;
      } else if (type == "path") {
        renderAlgo = 1;
      } else if (type == "") {
        renderAlgo = 2;
      } else if (type == "bdpt") {
        renderAlgo = 3;
      } else {
        renderAlgo = -1;
      }
    } else {
      renderAlgo = -1;
    }
    const char* prev = renderAlgo == -1 ? "" : RendererNames()[renderAlgo];
    if (ImGui::BeginCombo(_app->I18n("offline_render.algorithm"), prev)) {
      for (int i = 0; i < RendererNameCount(); i++) {
        bool isSelected = (i == renderAlgo);
        if (ImGui::Selectable(RendererNames()[i], isSelected)) {
          renderAlgo = i;
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
    ImGui::BeginDisabled(_app->IsOfflineRendering());
    if (ImGui::Button(_app->I18n("offline_render.start"))) {
      _app->StartOfflineRender();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!_app->IsOfflineRendering());
    if (ImGui::Button(_app->I18n("offline_render.stop"))) {
      _app->StartOfflineRender();
    }
    ImGui::EndDisabled();
    auto& data = _app->GetOfflineRender();
    if (data.ResultTex != 0) {
      auto tex = (size_t)data.ResultTex;
      ImGui::Image(
          reinterpret_cast<void*>(tex),
          ImVec2((float)data.Width, (float)data.Height));
    }
  }
}

}  // namespace Rad
