#pragma once

#include <rad/core/config_node.h>
#include <rad/core/asset.h>

#include <variant>

namespace Rad {

class UIManager;
class EditorApplication;

class OfflineConfig {
 public:
  OfflineConfig(EditorApplication* app, UIManager* ui) : _app(app), _ui(ui) {}
  virtual ~OfflineConfig() noexcept = default;

  virtual void OnGui() = 0;

 protected:
  EditorApplication* _app;
  UIManager* _ui;
};

}  // namespace Rad
