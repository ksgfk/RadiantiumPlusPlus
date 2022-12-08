#pragma once

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/core/asset.h>
#include <rad/core/factory.h>

#include <filesystem>
#include <list>

#include "ui_manager.h"

namespace Rad {

class EditorApplication;

Matrix4f ConfigNodeAsTransformf(ConfigNode node);
bool ConfigNodeTryReadRotatef(ConfigNode node, const std::string& name, Matrix3f& result);
Matrix3f ConfigNodeAsRotatef(ConfigNode node);

class SceneNode {
 public:
  SceneNode(EditorApplication* editor);

  SceneNode& AddChildBefore(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildAfter(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildLast(SceneNode&& node);
  void RemoveChild(SceneNode& child);
  void SetToWorldMatrix(const Matrix4f& toWorld);
  const std::list<SceneNode>& GetChildren() const { return _children; }
  size_t GetUniqueId() const { return _uniqueId; }

 private:
  void AfterAddChild(std::list<SceneNode>::iterator&);

  size_t _uniqueId;
  EditorApplication* _editor;
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
  std::string GetSceneName() const { return _sceneName; }
  SceneNode& GetRootNode() { return _root; }
  const SceneNode& GetRootNode() const { return _root; }
  bool IsSceneDirty() const { return _isSceneDirty; }
  void MarkSceneDirty() { _isSceneDirty = true; }
  size_t RequestUniqueId();

 private:
  void OnUpdate();

  Unique<UIManager> _ui;
  Share<spdlog::logger> _logger;

  bool _isWorkspaceActive{false};
  std::string _sceneName;
  Unique<FactoryManager> _factory;
  Unique<AssetManager> _asset;
  SceneNode _root{this};
  bool _isSceneDirty{false};
  size_t _nowId{0};
};

}  // namespace Rad
