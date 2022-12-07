#pragma once

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/core/asset.h>
#include <rad/core/factory.h>

#include <filesystem>
#include <list>

#include "ui_manager.h"

namespace Rad {

Matrix4f ConfigNodeAsTransformf(ConfigNode node);
bool ConfigNodeTryReadRotatef(ConfigNode node, const std::string& name, Matrix3f& result);
Matrix3f ConfigNodeAsRotatef(ConfigNode node);

class SceneNode {
 public:
  SceneNode() {
    _isDirty = false;
    _toWorld = Matrix4f::Identity();
    _toLocal = Matrix4f::Identity();
    _localToWorld = Matrix4f::Identity();
    _parent = nullptr;
  }

  SceneNode& AddChildBefore(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildAfter(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildLast(SceneNode&& node);
  void RemoveChild(SceneNode& child);
  void SetToWorldMatrix(const Matrix4f& toWorld);

 private:
  void AfterAddChild(std::list<SceneNode>::iterator&);

  bool _isDirty;
  Matrix4f _toWorld;
  Matrix4f _toLocal;
  Matrix4f _localToWorld;
  SceneNode* _parent;
  std::list<SceneNode> _children;
  std::list<SceneNode>::iterator _inParentPos;

 public:
  std::string Name;
};

class EditorApplication {
 public:
  EditorApplication(int argc, char** argv);
  ~EditorApplication() noexcept;
  EditorApplication(const EditorApplication&) = delete;

  void Run();

  void OpenWorkspace(const std::filesystem::path& filePath);
  bool IsWorkspaceActive() const;

 private:
  Unique<UIManager> _ui;
  Share<spdlog::logger> _logger;

  bool _isWorkspaceActive{false};
  Unique<FactoryManager> _factory;
  Unique<AssetManager> _asset;
  SceneNode _root{};
};

}  // namespace Rad
