#include <rad/offline/editor/gui_hierarchy.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <rad/offline/editor/application.h>

namespace Rad {

GuiHierarchy::GuiHierarchy(Application* app) : GuiObject(app, 0, true, "hierarchy_panel") {}

void GuiHierarchy::OnGui() {
  using NodeRecursive = GuiHierarchy::NodeRecursive;
  if (!IsOpen) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(0, 50), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(350, 700), ImGuiCond_Once);
  ScopeGuard _([]() { ImGui::End(); });
  if (ImGui::Begin(_app->I18n("hierarchy.title"), &IsOpen, 0)) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    auto region = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("_hierarchy_main", ImVec2(region.x, region.y * 0.5f), true, ImGuiWindowFlags_HorizontalScrollbar)) {
      if (ImGui::Button(_app->I18n("hierarchy.add_node"))) {
        ShapeNode* target = NowSelect == nullptr ? &_app->GetRoot() : NowSelect;
        ShapeNode newNode = _app->NewNode();
        newNode.Name = fmt::format("node {}", newNode.Id);
        ShapeNode& added = target->AddChildLast(std::move(newNode));
        NowSelect = &added;
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(NowSelect == nullptr);
      if (ImGui::Button(_app->I18n("hierarchy.rm_node"))) {
        ShapeNode temp = std::move(*NowSelect);
        NowSelect = nullptr;
        temp.Parent->RemoveChild(temp);
      }
      ImGui::EndDisabled();
      Cache.emplace(NodeRecursive{&_app->GetRoot(), 0});
      Int32 lastDepth = 0;
      bool moveNode = false;
      ShapeNode* fromNode{nullptr};
      ShapeNode* toNode{nullptr};
      while (!Cache.empty()) {
        auto [node, depth] = Cache.top();
        Cache.pop();
        for (Int32 i = 0; i < lastDepth - depth; i++) {
          ImGui::TreePop();
        }
        lastDepth = depth;
        ImGuiTreeNodeFlags nodeFlag = 0;
        nodeFlag |= ImGuiTreeNodeFlags_OpenOnArrow;
        nodeFlag |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
        nodeFlag |= ImGuiTreeNodeFlags_SpanAvailWidth;
        auto&& children = node->Children;
        bool isLeaf = children.empty();
        if (isLeaf) {
          nodeFlag |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (node->Parent != nullptr && NowSelect == node) {
          nodeFlag |= ImGuiTreeNodeFlags_Selected;
        }
        bool isOpen = ImGui::TreeNodeEx((void*)node->Id, nodeFlag, "%s", node->Name.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
          if (node->Parent != nullptr) {
            NowSelect = NowSelect == node ? nullptr : node;
          }
        }
        ImGui::PushID(node);
        if (node->Parent != nullptr) {
          ImGuiDragDropFlags dragFlag = 0;
          dragFlag |= ImGuiDragDropFlags_SourceNoPreviewTooltip;
          if (ImGui::BeginDragDropSource(dragFlag)) {
            ImGui::SetDragDropPayload("drag_scene_obj", &node, sizeof(void*));
            ImGui::EndDragDropSource();
          }
        }
        if (ImGui::BeginDragDropTarget()) {
          auto payload = ImGui::AcceptDragDropPayload("drag_scene_obj");
          if (payload != nullptr) {
            ShapeNode* from = *reinterpret_cast<ShapeNode**>(payload->Data);
            if (from != node && from->Parent != node) {
              moveNode = true;
              fromNode = from;
              toNode = node;
              NowSelect = nullptr;
            }
          }
          ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
        if (isOpen) {
          for (auto it = children.rbegin(); it != children.rend(); it++) {
            Cache.emplace(NodeRecursive{&(*it), depth + 1});
          }
        }
      }
      for (Int32 i = 0; i < lastDepth; i++) {
        ImGui::TreePop();
      }
      if (moveNode) {
        ShapeNode temp = std::move(*fromNode);
        temp.Parent->RemoveChild(temp);
        toNode->AddChildLast(std::move(temp));
      }
    }
    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::BeginChild("_hierarchy_info", ImGui::GetContentRegionAvail(), true, ImGuiWindowFlags_HorizontalScrollbar)) {
      if (NowSelect != nullptr) {
        ImGui::Text("ID: %lld", NowSelect->Id);
        ImGui::InputText(_app->I18n("name"), &NowSelect->Name);
        DrawPSR(&NowSelect->Position, &NowSelect->Scale, &NowSelect->Rotation);
        {
          auto shape = NowSelect->ShapeAsset.Ptr;
          auto comboPreview = shape == nullptr ? "" : shape->Name.c_str();
          if (ImGui::BeginCombo(_app->I18n("hierarchy.shape"), comboPreview)) {
            auto&& assets = _app->GetAssets();
            TempFilter.clear();
            for (auto&& i : assets) {
              if (i.second->Type == 0) {
                TempFilter.emplace_back(i.second.get());
              }
            }
            for (auto i : TempFilter) {
              bool isSelected = (i == shape);
              if (ImGui::Selectable(i->Name.c_str(), isSelected)) {
                NowSelect->ShapeAsset = EditorAssetGuard(i);
              }
              if (isSelected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }
        }
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }
}

}  // namespace Rad
