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

GuiLoadAsset::GuiLoadAsset(Application* app) : GuiObject(app, -999, true, "_load_asset") {}

void GuiLoadAsset::OnGui() {
  ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_Appearing);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("asset_panel.load"), nullptr)) {
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
        NewAssetLocation = std::filesystem::relative(path, _app->GetWorkRoot()).string();
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
      _isAlive = false;
    }
    ImGui::SameLine();
    if (ImGui::Button(_app->I18n("cancel"))) {
      _isAlive = false;
    }
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
  ImGui::SetNextWindowPos(ImVec2(520, 580), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("asset_panel.title"), &IsOpen, flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::Button(_app->I18n("asset_panel.load"))) {
        if (!_app->FindUi("_load_asset")) {
          auto load = std::make_unique<GuiLoadAsset>(_app);
          _app->AddGui(std::move(load));
        }
      }
      ImGui::EndMenuBar();
    }
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
  ImGuiWindowFlags listFlag = ImGuiWindowFlags_HorizontalScrollbar;
  {
    auto region = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("_List1", ImVec2(region.x * 0.5f, region.y), true, listFlag)) {
      ImGui::BeginTable("_title", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%s", _app->I18n("name"));
      ImGui::TableNextColumn();
      ImGui::Text("%s", _app->I18n("asset_panel.type"));
      for (auto&& i : _app->GetAssets()) {
        auto&& [name, ins] = i;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        bool isSelected = SelectedAsset == name;
        bool isClick = ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns);
        ImGui::TableNextColumn();
        ImGui::Text("%s", AssetNames()[ins->Type]);
        if (isClick) {
          if (isSelected) {
            SelectedAsset.clear();
          } else {
            SelectedAsset = name;
          }
        }
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
  }
  ImGui::SameLine();
  {
    auto region = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("_List2", ImVec2(region.x, region.y), true, listFlag)) {
      if (SelectedAsset.size() > 0) {
        auto iter = _app->GetAssets().find(SelectedAsset);
        if (iter != _app->GetAssets().end()) {
          auto&& assIns = iter->second;
          ImGui::Text("%s: ", _app->I18n("name"));
          ImGui::SameLine();
          ImGui::Text("%s", assIns->Name.c_str());
          ImGui::Text("%s: ", _app->I18n("asset_panel.type"));
          ImGui::SameLine();
          ImGui::Text("%s", AssetNames()[assIns->Type]);
          ImGui::Text("%s: ", _app->I18n("asset_panel.location"));
          ImGui::SameLine();
          ImGui::Text("%s", assIns->Loaction.string().c_str());
          ImGui::Text("%s: ", _app->I18n("asset_panel.ref"));
          ImGui::SameLine();
          ImGui::Text("%d", assIns->Reference);
        }
      }
    }
    ImGui::EndChild();
  }
  ImGui::PopStyleVar();
}

}  // namespace Rad
