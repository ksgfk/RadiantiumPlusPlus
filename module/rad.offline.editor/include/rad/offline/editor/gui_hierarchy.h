#pragma once

#include <stack>
#include <random>

#include "gui_object.h"

namespace Rad {

class ShapeNode;
class EditorAsset;

class GuiHierarchy : public GuiObject {
 public:
  explicit GuiHierarchy(Application* app);
  ~GuiHierarchy() noexcept override = default;

  void OnGui() override;

  struct NodeRecursive {
    ShapeNode* Node;
    int Depth;
  };

  bool IsOpen{true};
  std::stack<NodeRecursive> Cache;
  ShapeNode* NowSelect{nullptr};
  std::mt19937 Rng;
  std::vector<EditorAsset*> TempFilter;
};

}  // namespace Rad
