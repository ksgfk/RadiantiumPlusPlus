#include <rad/offline/editor/gui_main_bar.h>

#include <imgui.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiMainBar::GuiMainBar(Application* app)
    : GuiObject(app, 0, true),
      _logger(app->GetLogger()) {}

void GuiMainBar::OnGui() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu(_app->I18n("main_menu_bar.file"))) {
      if (ImGui::MenuItem(_app->I18n("main_menu_bar.file.new_scene"))) {
        _logger->debug("new scene");
        auto _menuBarFb = std::make_unique<ImGui::FileBrowser>(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
        // _menuBarFb->SetPwd(_workRoot);
        _menuBarFb->SetTitle(_app->I18n("main_menu_bar.file.open_scene.file_dialog_title"));
        _menuBarFb->SetInpuTextHint(_app->I18n("main_menu_bar.file.open_scene.file_dialog_filename"));
        _menuBarFb->SetButtonOkHint(_app->I18n("ok"));
        _menuBarFb->SetButtonCancelHint(_app->I18n("cancel"));
        _menuBarFb->Open();
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
        ImGui::ColorEdit3(_app->I18n("main_menu_bar.setting.background color"), _app->BackgroundColor().data(), ImGuiColorEditFlags_NoLabel);
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
    ImGui::EndMainMenuBar();
  }
}

}  // namespace Rad
