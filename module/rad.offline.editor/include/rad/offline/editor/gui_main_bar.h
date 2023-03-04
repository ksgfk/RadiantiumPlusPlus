#pragma once

#include <rad/core/logger.h>

#include "gui_object.h"

namespace Rad {

class GuiMainBar : public GuiObject {
 public:
  explicit GuiMainBar(Application* app);
  ~GuiMainBar() noexcept override = default;

  void OnGui() override;

 private:
  Share<spdlog::logger> _logger;
};

}  // namespace Rad
