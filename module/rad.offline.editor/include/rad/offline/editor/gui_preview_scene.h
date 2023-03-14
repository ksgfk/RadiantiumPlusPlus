#pragma once

#include "gui_object.h"

namespace Rad {

class GuiPreviewScene : public GuiObject {
 public:
  explicit GuiPreviewScene(Application* app);
  ~GuiPreviewScene() noexcept override = default;

  void OnGui() override;

  bool IsOpen = true;
};

}  // namespace Rad
