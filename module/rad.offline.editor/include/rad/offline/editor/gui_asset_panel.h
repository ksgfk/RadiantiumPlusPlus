#pragma once

#include "gui_object.h"

namespace Rad {

class GuiAssetPanel : public GuiObject {
 public:
  explicit GuiAssetPanel(Application* app);
  ~GuiAssetPanel() noexcept override = default;

  void OnGui() override;

  bool IsOpen;
};

}  // namespace Rad
