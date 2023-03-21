#pragma once

#include "gui_object.h"

namespace Rad {

class GuiOfflineRenderPanel : public GuiObject {
 public:
  explicit GuiOfflineRenderPanel(Application* app);
  ~GuiOfflineRenderPanel() noexcept override = default;

  void OnGui() override;

  bool IsOpen{true};
  bool IsRendering{false};
};

}  // namespace Rad
