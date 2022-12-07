#pragma once

#include <rad/core/types.h>

#include "ui_manager.h"

namespace Rad {

class EditorApplication {
 public:
  EditorApplication(int argc, char** argv);
  ~EditorApplication() noexcept;
  EditorApplication(const EditorApplication&) = delete;

  void Run();

 private:
  Unique<UIManager> _ui;
};

}  // namespace Rad
