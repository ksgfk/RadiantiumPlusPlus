#pragma once

#include <functional>

#include <imgui.h>
#include <imfilebrowser.h>

#include "gui_object.h"

namespace Rad {

class GuiFileBrowserWrapper : public GuiObject {
 public:
  explicit GuiFileBrowserWrapper(Application* app, ImGui::FileBrowser&& browser);
  ~GuiFileBrowserWrapper() noexcept override = default;

  void OnStart() override;
  void OnGui() override;

  ImGui::FileBrowser& GetBrowser() { return _browser; }

 private:
  ImGui::FileBrowser _browser;

 public:
  std::function<void(GuiFileBrowserWrapper&)> OnSelected;
};

}  // namespace Rad
