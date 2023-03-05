#pragma once

#include "gui_object.h"

namespace Rad {

class GuiAssetLoadFailMessageBox : public GuiObject {
 public:
  explicit GuiAssetLoadFailMessageBox(Application* app);
  ~GuiAssetLoadFailMessageBox() noexcept override = default;

  void OnStart() override;
  void OnGui() override;

  std::string Message;
};

class GuiAssetPanel : public GuiObject {
 public:
  explicit GuiAssetPanel(Application* app);
  ~GuiAssetPanel() noexcept override = default;

  void OnGui() override;

  bool IsOpen;
  std::string NewAssetName;
  int NewAssetType;
  std::string NewAssetLocation;

  std::string SelectedAsset;
};

}  // namespace Rad
