#include <rad/offline/editor/gui_main_bar.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

#include <rad/offline/editor/gui_file_browser_wrapper.h>
#include <rad/offline/editor/gui_asset_panel.h>
#include <rad/offline/editor/gui_hierarchy.h>

namespace Rad {

GuiMainBar::GuiMainBar(Application* app)
    : GuiObject(app, 0, true, "main_bar"),
      _logger(app->GetLogger()) {}

void GuiMainBar::OnGui() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu(_app->I18n("main_menu_bar.file"))) {
      if (ImGui::MenuItem(_app->I18n("main_menu_bar.file.new_scene"))) {
        _logger->debug("new scene");
        ImGui::FileBrowser browser(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
        browser.SetPwd(_app->GetWorkRoot());
        browser.SetTitle(_app->I18n("main_menu_bar.file.open_scene.file_dialog_title"));
        browser.SetInpuTextHint(_app->I18n("main_menu_bar.file.open_scene.file_dialog_filename"));
        browser.SetButtonOkHint(_app->I18n("ok"));
        browser.SetButtonCancelHint(_app->I18n("cancel"));
        auto wrapper = std::make_unique<GuiFileBrowserWrapper>(_app, std::move(browser));
        wrapper->OnSelected = [=](GuiFileBrowserWrapper& wrapper) {
          auto&& b = wrapper.GetBrowser();
          auto path = b.GetSelected();
          _logger->info("select {}", path.string());
          if (path.has_extension()) {
            path.replace_extension(".json");
          } else {
            path += ".json";
          }
          _app->NewScene(path);
        };
        _app->AddGui(std::move(wrapper));
      }
      if (ImGui::MenuItem(_app->I18n("main_menu_bar.file.open_scene"))) {
        _logger->info("open");
      }
      if (ImGui::MenuItem(_app->I18n("main_menu_bar.file.save_scene"))) {
        _logger->info("save");
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(_app->I18n("main_menu_bar.setting"))) {
      if (ImGui::BeginMenu(_app->I18n("main_menu_bar.setting.background color"))) {
        ImGui::ColorEdit3(_app->I18n("main_menu_bar.setting.background color"), _app->GetBackgroundColor().data(), ImGuiColorEditFlags_NoLabel);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu(_app->I18n("main_menu_bar.setting.switch_language"))) {
        for (const auto& iter : std::filesystem::directory_iterator(std::filesystem::current_path() / "i18n")) {
          if (!iter.is_regular_file()) {
            continue;
          }
          auto filePath = iter.path();
          auto fileName = filePath.filename().string();
          std::transform(fileName.begin(), fileName.end(), fileName.begin(),
                         [](unsigned char c) { return std::tolower(c); });
          if (fileName == "zh_cn_utf8.json") {
            if (ImGui::MenuItem("中文")) {
              _app->LoadI18n(filePath);
              break;
            }
          } else if (fileName == "en_us.json") {
            if (ImGui::MenuItem("English")) {
              _app->LoadI18n(filePath);
              break;
            }
          } else {
            if (ImGui::MenuItem(fileName.c_str())) {
              _app->LoadI18n(filePath);
              break;
            }
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (_app->HasWorkspace()) {
      if (ImGui::BeginMenu(_app->I18n("main_menu_bar.window"))) {
        {
          auto assetPanel = _app->FindUi("asset_panel");
          if (assetPanel) {
            auto ptr = static_cast<GuiAssetPanel*>(*assetPanel);
            if (ImGui::MenuItem(_app->I18n("asset_panel.title"), nullptr, ptr->IsOpen)) {
              ptr->IsOpen = !ptr->IsOpen;
            }
          }
        }
        {
          auto hierarchyPanel = _app->FindUi("hierarchy_panel");
          if (hierarchyPanel) {
            auto ptr = static_cast<GuiHierarchy*>(*hierarchyPanel);
            if (ImGui::MenuItem(_app->I18n("hierarchy.title"), nullptr, ptr->IsOpen)) {
              ptr->IsOpen = !ptr->IsOpen;
            }
          }
        }
        ImGui::EndMenu();
      }
    }
    ImGui::EndMainMenuBar();
  }
}

}  // namespace Rad
