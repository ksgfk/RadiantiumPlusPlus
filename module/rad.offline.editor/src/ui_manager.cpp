#include <rad/offline/editor/ui_manager.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <rad/core/logger.h>
#include <rad/realtime/window.h>

#include <algorithm>
#include <filesystem>

namespace Rad {

class MainMenu;
class FileBrowser;
class MessageBox;

class MainMenu final : public GuiItem {
 public:
  MainMenu() {
    _priority = -9999;
    _isAlive = true;
  }
  ~MainMenu() noexcept override = default;

  void OnGui() override;
};

class FileBrowser final : public GuiItem {
 public:
  FileBrowser(
      const std::string& title,
      const std::filesystem::path& current,
      std::function<void(const std::vector<std::filesystem::path>&)>&& on = nullptr,
      bool selectFile = true,
      bool selectOneFile = true)
      : GuiItem(),
        _title(title), _currentPath(current),
        _onSelect(std::move(on)),
        _isSelectFile(selectFile), _isSelectOneFile(selectOneFile) {
    _priority = 999;
    _isAlive = true;
    RefreshDir();
  }
  ~FileBrowser() noexcept override = default;

  void OnStart() override;
  void OnGui() override;

 public:
  Int32 Width{800}, Height{600};

 private:
  struct DirEntry {
    std::string Path;
    bool IsFile;
    bool IsDirectory;
  };

  struct DirectoryEntry {
    std::string Name;
    bool IsSelected{false};
  };
  struct FileEntry {
    std::string Name;
    bool IsSelected{false};
  };

  void RefreshDir();

  std::string _title;
  std::filesystem::path _currentPath;
  std::function<void(const std::vector<std::filesystem::path>&)> _onSelect;
  bool _isSelectFile;
  bool _isSelectOneFile;

  std::string _inputPathBuf;

  std::vector<DirectoryEntry> _cacheDirEntries;
  std::vector<FileEntry> _cacheFileEntries;

  Unique<MessageBox> _box;
  bool _isShowBox{false};
};

class MessageBox final : public GuiItem {
 public:
  MessageBox(
      const std::string& msg,
      const std::string& title = "提示",
      std::function<void(void)>&& onOk = nullptr,
      std::function<void(void)>&& onCancel = nullptr)
      : GuiItem(), _msg(msg), _title(title), _onOk(std::move(onOk)), _onCancel(std::move(onCancel)) {
    _isAlive = true;
  }
  ~MessageBox() noexcept override = default;

  void OnStart() override;
  void OnGui() override;

 private:
  std::string _msg;
  std::string _title;
  std::function<void(void)> _onOk;
  std::function<void(void)> _onCancel;
};

void MainMenu::OnGui() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("文件")) {
      if (ImGui::MenuItem("从文件打开工作区")) {
        Unique<FileBrowser> v = std::make_unique<FileBrowser>(
            "打开",
            std::filesystem::current_path(),
            [](auto&& i) {
              for (auto&& j : i) {
                Logger::Get()->info("{}", j.u8string());
              }
            });
        _ui->AddGui(std::move(v));
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void FileBrowser::OnStart() {
  ImGui::OpenPopup(_title.c_str());

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2((float)Width, (float)Height));
}

void FileBrowser::OnGui() {
  _isShowBox = false;
  ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
  if (ImGui::BeginPopupModal(_title.c_str(), nullptr, flags)) {
    if (ImGui::Button("刷新")) {
      RefreshDir();
    }
    ImGui::SameLine();
    ImGuiInputTextFlags inputFlags = 0;
    inputFlags |= ImGuiInputTextFlags_AutoSelectAll;
    inputFlags |= ImGuiInputTextFlags_EnterReturnsTrue;
    bool completion = ImGui::InputText(
        "##file_browser_path",
        _inputPathBuf.data(), _inputPathBuf.size(),
        inputFlags);
    if (completion) {
      size_t pos = _inputPathBuf.find('\0');
      if (pos != std::string::npos) {
        _inputPathBuf.resize(pos);
      }
      _currentPath = std::filesystem::path(_inputPathBuf);
      Logger::Get()->info("hummmmm {}", _inputPathBuf);
      RefreshDir();
    }
    if (ImGui::BeginListBox("##fb_fls", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing()))) {
      bool isSkip = false;
      ImGuiSelectableFlags selectFlags = ImGuiSelectableFlags_AllowDoubleClick;
      if (ImGui::Selectable("..", false, selectFlags)) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          _currentPath = _currentPath.parent_path();
          RefreshDir();
          isSkip = true;
        }
      }
      for (auto&& i : _cacheDirEntries) {
        if (isSkip) {
          break;
        }
        auto name = fmt::format("[文件夹] {}", i.Name);
        if (ImGui::Selectable(name.c_str(), i.IsSelected, selectFlags)) {
          if (!_isSelectFile) {
            i.IsSelected = !i.IsSelected;
          }
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            auto nextDirName = _currentPath / i.Name;
            if (std::filesystem::exists(nextDirName)) {
              std::filesystem::directory_entry nextDirInfo(nextDirName);
              if (nextDirInfo.is_directory()) {
                _currentPath = nextDirName;
                RefreshDir();
                isSkip = true;
              }
            }
          }
        }
      }
      FileEntry* thisRountSelect = nullptr;
      for (auto&& i : _cacheFileEntries) {
        if (isSkip) {
          break;
        }
        auto name = fmt::format("[文件] {}", i.Name);
        if (ImGui::Selectable(name.c_str(), i.IsSelected, selectFlags)) {
          if (_isSelectFile) {
            thisRountSelect = &i;
            i.IsSelected = !i.IsSelected;
          }
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            auto selectName = _currentPath / i.Name;
            if (std::filesystem::exists(selectName)) {
              if (_isSelectFile && _onSelect != nullptr) {
                _onSelect(std::vector<std::filesystem::path>{selectName});
                _isAlive = false;
              }
            } else {
              RefreshDir();
            }
            isSkip = true;
            break;
          }
        }
      }
      if (_isSelectOneFile) {
        if (thisRountSelect != nullptr) {
          for (auto&& i : _cacheFileEntries) {
            i.IsSelected = false;
          }
          (*thisRountSelect).IsSelected = true;
        }
      }
      ImGui::EndListBox();
    }

    if (ImGui::Button("确认", ImVec2(120, 0))) {
      std::vector<std::filesystem::path> done;
      if (_isSelectFile) {
        for (auto&& i : _cacheFileEntries) {
          if (!i.IsSelected) {
            continue;
          }
          auto full = _currentPath / i.Name;
          if (std::filesystem::exists(full)) {
            done.emplace_back(std::move(full));
          }
          if (_isSelectOneFile) {
            break;
          }
        }
      } else {
        for (auto&& i : _cacheDirEntries) {
          if (!i.IsSelected) {
            continue;
          }
          auto full = _currentPath / i.Name;
          if (std::filesystem::exists(full)) {
            done.emplace_back(std::move(full));
            break;
          }
        }
      }
      if (done.size() > 0) {
        _onSelect(done);
        _isAlive = false;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("取消", ImVec2(120, 0))) {
      _isAlive = false;
    }

    if (_box != nullptr && !_isShowBox) {
      _box->OnGui();
      _isShowBox = true;
      if (!_box->IsAlive()) {
        _box = nullptr;
      }
    }

    ImGui::EndPopup();
  }
}

void FileBrowser::RefreshDir() {
  _cacheDirEntries.clear();
  _cacheFileEntries.clear();
  if (!std::filesystem::exists(_currentPath)) {
    _box = std::make_unique<MessageBox>(fmt::format("不存在目录: {}", _currentPath.u8string()));
    _box->OnStart();
    _currentPath = std::filesystem::current_path();
  } else {
    _inputPathBuf = _currentPath.u8string();
    _inputPathBuf.resize(512, '\0');
  }
  for (auto&& p : std::filesystem::directory_iterator(_currentPath)) {
    auto name = p.path().filename().u8string();
    if (p.is_directory()) {
      _cacheDirEntries.emplace_back(DirectoryEntry{name});
    }
    if (p.is_regular_file()) {
      _cacheFileEntries.emplace_back(FileEntry{name});
    }
  }
  auto nameCmp = [](auto&& l, auto&& r) {
    return l.Name < r.Name;
  };
  std::sort(_cacheDirEntries.begin(), _cacheDirEntries.end(), nameCmp);
  std::sort(_cacheFileEntries.begin(), _cacheFileEntries.end(), nameCmp);
}

void MessageBox::OnStart() {
  ImGui::OpenPopup(_title.c_str());
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

void MessageBox::OnGui() {
  if (ImGui::BeginPopupModal(_title.c_str(), nullptr, 0)) {
    auto winSize_ = ImGui::GetWindowSize();
    auto textSize_ = ImGui::CalcTextSize(_msg.c_str());
    Vector2f winSize(winSize_.x, winSize_.y);
    Vector2f texSize(textSize_.x, textSize_.y);
    Vector2f p = (winSize - texSize) * 0.5f;
    ImGui::SetCursorPos(ImVec2(p.x(), p.y()));
    ImGui::Text("%s", _msg.c_str());
    if (ImGui::Button("确认")) {
      if (_onOk != nullptr) {
        _onOk();
      }
      _isAlive = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("取消")) {
      if (_onCancel != nullptr) {
        _onCancel();
      }
      _isAlive = false;
    }
    ImGui::EndPopup();
  }
}

UIManager::UIManager(EditorApplication* editor) : _editor(editor) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF("fonts/SourceHanSerifCN-Medium.ttf", 20, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
  io.Fonts->Build();
  ImGui::StyleColorsLight();
  ImGui::GetStyle().ScaleAllSizes(1);
  ImGui_ImplGlfw_InitForOpenGL(Rad::RadWindowHandlerGlfw(), false);
  ImGui_ImplOpenGL3_Init("#version 450 core");

  AddWindowFocusEventGlfw(ImGui_ImplGlfw_WindowFocusCallback);
  AddCursorEnterEventGlfw(ImGui_ImplGlfw_CursorEnterCallback);
  AddCursorPosEventGlfw([](auto w, auto&& p) { ImGui_ImplGlfw_CursorPosCallback(w, p.x(), p.y()); });
  AddMouseButtonEventGlfw(ImGui_ImplGlfw_MouseButtonCallback);
  AddScrollEventGlfw([](auto w, auto&& p) { ImGui_ImplGlfw_ScrollCallback(w, p.x(), p.y()); });
  AddKeyEventGlfw(ImGui_ImplGlfw_KeyCallback);
  AddCharEventGlfw(ImGui_ImplGlfw_CharCallback);
  AddMonitorEventGlfw(ImGui_ImplGlfw_MonitorCallback);

  AddGui(std::make_unique<MainMenu>());
}

UIManager::~UIManager() noexcept {
  ImGui::DestroyContext();
}

void UIManager::NewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void UIManager::OnGui() {
  // 我们应该保证相同优先级，按插入顺序倒序显示
  auto uiCmp = [](const auto& l, const auto& r) {
    return l->GetPriority() > r->GetPriority();
  };
  _drawList.clear();
  _itemList.swap(_drawList);
  std::stable_sort(_startList.begin(), _startList.end(), uiCmp);
  for (auto&& i : _startList) {
    i->OnStart();
    i->OnGui();
    if (i->IsAlive()) {
      _itemList.emplace_back(std::move(i));
    }
  }
  _startList.clear();
  std::stable_sort(_drawList.begin(), _drawList.end(), uiCmp);
  for (auto&& i : _drawList) {
    if (i->IsAlive()) {
      i->OnGui();
      _itemList.emplace_back(std::move(i));
    }
  }

  ImGui::ShowDemoWindow();
}

void UIManager::Draw() {
  ImGui::Render();
  Rad::Vector2i size = Rad::RadGetFrameBufferSizeGlfw();
  glViewport(0, 0, size.x(), size.y());
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::AddGui(Unique<GuiItem> ui) {
  ui->_editor = _editor;
  ui->_ui = this;
  _startList.emplace_back(std::move(ui));
}

}  // namespace Rad
