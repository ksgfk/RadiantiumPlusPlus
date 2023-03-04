#include <rad/offline/editor/gui_file_browser_wrapper.h>

namespace Rad {

GuiFileBrowserWrapper::GuiFileBrowserWrapper(Application* app, ImGui::FileBrowser&& browser)
    : GuiObject(app, -999, true,"_file_browser_wrapper"),
      _browser(std::move(browser)) {}

void GuiFileBrowserWrapper::OnStart() {
  _browser.Open();
}

void GuiFileBrowserWrapper::OnGui() {
  _browser.Display();
  if (_browser.HasSelected()) {
    if (OnSelected != nullptr) {
      OnSelected(*this);
      _isAlive = false;
    }
  }
  if (!_browser.IsOpened()) {
    _isAlive = false;
  }
}

}  // namespace Rad
