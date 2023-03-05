#include <rad/offline/editor/gui_asset_panel.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <rad/offline/editor/application.h>
#include <rad/offline/editor/gui_file_browser_wrapper.h>

namespace Rad {

GuiAssetLoadFailMessageBox::GuiAssetLoadFailMessageBox(Application* app) : GuiObject(app, -999, true, "_asset_fail_msgbox") {}

void GuiAssetLoadFailMessageBox::OnStart() {
  ImGui::OpenPopup(_app->I18n("asset_panel.load.fail"));
}

void GuiAssetLoadFailMessageBox::OnGui() {
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(_app->I18n("asset_panel.load.fail"), nullptr)) {
    auto format = fmt::format(_app->I18n("asset_panel.load.reason"), Message.c_str());
    ImGui::Text("%s", format.c_str());
    if (ImGui::Button(_app->I18n("close"))) {
      _isAlive = false;
    }
    ImGui::EndPopup();
  }
}

GuiAssetPanel::GuiAssetPanel(Application* app)
    : GuiObject(app, 1, true, "asset_panel"),
      IsOpen(true),
      NewAssetType(-1) {}

void GuiAssetPanel::OnGui() {
  if (!IsOpen) {
    return;
  }
  ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("asset_panel.title"), &IsOpen, flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu(_app->I18n("asset_panel.load"))) {
        ImGui::InputText(_app->I18n("asset_panel.load.unique_name"), &NewAssetName, ImGuiInputTextFlags_AutoSelectAll);
        auto assetNames = AssetNames();
        auto comboPreview = NewAssetType < 0 ? "" : assetNames[NewAssetType];
        if (ImGui::BeginCombo(_app->I18n("asset_panel.load.type"), comboPreview)) {
          for (size_t n = 0; n < AssetNameCount(); n++) {
            bool isSelected = (NewAssetType == n);
            if (ImGui::Selectable(assetNames[n], isSelected)) {
              NewAssetType = (int)n;
            }
            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        auto format = fmt::format(_app->I18n("asset_panel.load.location"), NewAssetLocation);
        ImGui::Text("%s", format.data());
        ImGui::SameLine();
        if (ImGui::Button("..")) {
          ImGui::FileBrowser browser(0);
          browser.SetPwd(_app->GetWorkRoot());
          browser.SetTitle(_app->I18n("main_menu_bar.file.open_scene.file_dialog_title"));
          browser.SetInpuTextHint(_app->I18n("main_menu_bar.file.open_scene.file_dialog_filename"));
          browser.SetButtonOkHint(_app->I18n("ok"));
          browser.SetButtonCancelHint(_app->I18n("cancel"));
          auto wrapper = std::make_unique<GuiFileBrowserWrapper>(_app, std::move(browser));
          wrapper->OnSelected = [&](GuiFileBrowserWrapper& wrapper) {
            auto&& b = wrapper.GetBrowser();
            auto path = b.GetSelected();
            NewAssetLocation = path.string();
          };
          _app->AddGui(std::move(wrapper));
        }
        if (ImGui::Button(_app->I18n("asset_panel.load"))) {
          auto [succ, result] = _app->LoadAsset(NewAssetName, std::filesystem::path(NewAssetLocation), NewAssetType);
          if (!succ) {
            auto msgBox = std::make_unique<GuiAssetLoadFailMessageBox>(_app);
            msgBox->Message = result;
            _app->AddGui(std::move(msgBox));
          }
        }
        ImGui::SameLine();
        if (ImGui::Button(_app->I18n("asset_panel.clear"))) {
          NewAssetName.clear();
          NewAssetLocation.clear();
          NewAssetType = -1;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
  }
}

}  // namespace Rad
