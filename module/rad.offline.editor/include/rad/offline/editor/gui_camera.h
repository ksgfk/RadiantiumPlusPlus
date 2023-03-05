#pragma once

#include "gui_object.h"

namespace Rad {

class GuiCamera : public GuiObject {
 public:
  explicit GuiCamera(Application* app);
  ~GuiCamera() noexcept override = default;

  void OnGui() override;

  bool IsOpen{true};
};

}  // namespace Rad
