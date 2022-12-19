#pragma once

#include <rad/core/types.h>
#include <rad/core/config_node.h>

#include <imgui.h>

#include <string>
#include <vector>

namespace Rad {

class EditorApplication;
class UIManager;
class SceneNode;

struct UIContext {
  EditorApplication* App;
  UIManager* UI;
};

class GuiItem {
 public:
  virtual ~GuiItem() noexcept = default;

  virtual void OnStart() {}
  virtual void OnGui() = 0;

  bool IsAlive() const { return _isAlive; }
  int GetPriority() const { return _priority; }

 protected:
  EditorApplication* _editor;
  UIManager* _ui;
  int _priority{0};
  bool _isAlive{false};

  friend class UIManager;
};

class UIManager {
 public:
  UIManager(EditorApplication*);
  ~UIManager() noexcept;
  UIManager(const UIManager&) = delete;

  void NewFrame();
  void OnGui();
  void Draw();
  void Reset();

  void AddGui(Unique<GuiItem>);

 private:
  Share<spdlog::logger> _logger;
  EditorApplication* _editor;
  std::vector<Unique<GuiItem>> _itemList;
  std::vector<Unique<GuiItem>> _drawList;
  std::vector<Unique<GuiItem>> _startList;
};

void SetupNodeShape(EditorApplication* app, UIManager* ui, SceneNode* node, ConfigNode cfg);
void SetupNodeBsdf(EditorApplication* app, UIManager* ui, SceneNode* node, ConfigNode cfg);

}  // namespace Rad
