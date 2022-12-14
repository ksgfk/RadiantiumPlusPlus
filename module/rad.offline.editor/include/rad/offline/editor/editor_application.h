#pragma once

#include <rad/core/types.h>
#include <rad/core/logger.h>
#include <rad/core/asset.h>
#include <rad/core/factory.h>

#include <filesystem>
#include <list>

#include "ui_manager.h"
#include "coms.h"

namespace Rad {

class EditorApplication;

Matrix4f ConfigNodeAsTransformf(ConfigNode node);
bool ConfigNodeTryReadRotatef(ConfigNode node, const std::string& name, Matrix3f& result);
Matrix3f ConfigNodeAsRotatef(ConfigNode node);
std::tuple<Matrix3f, Eigen::Quaternionf, Vector3f> DecomposeTransformf(const Matrix4f&);
Vector3f MatToEulerAngleZXY(const Matrix3f& mat);
Eigen::Quaternionf EulerAngleToQuatZXY(const Vector3f& eulerAngle);
Matrix3f EulerAngleToMatZXY(const Vector3f& eulerAngle);

class SceneNode {
 public:
  SceneNode(EditorApplication* editor);

  SceneNode& AddChildBefore(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildAfter(const SceneNode& pos, SceneNode&& node);
  SceneNode& AddChildLast(SceneNode&& node);
  void RemoveChild(SceneNode& child);
  void SetToWorldMatrix(const Matrix4f& toWorld);
  const std::list<SceneNode>& GetChildren() const { return _children; }
  std::list<SceneNode>& GetChildren() { return _children; }
  SceneNode* GetParent() const { return _parent; }
  size_t GetUniqueId() const { return _uniqueId; }
  bool IsSelect() const { return _isSelect; }
  void SetSelect(bool isSelect) { _isSelect = isSelect; }
  SceneNode* PreviousNode();
  SceneNode* NextNode();

 private:
  void AfterAddChild(std::list<SceneNode>::iterator&);

  size_t _uniqueId{0};
  EditorApplication* _editor{nullptr};
  Matrix4f _toWorld;
  Matrix4f _toLocal;

 public:
  Vector3f Position;
  Vector3f Scale;
  Eigen::Quaternionf Rotation;

 private:
  SceneNode* _parent{nullptr};
  std::list<SceneNode> _children;
  std::list<SceneNode>::iterator _inParentPos{};
  bool _isSelect{false};

 public:
  std::string Name;

  Unique<OfflineConfig> Shape;
  Unique<OfflineConfig> Bsdf;
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
  SceneNode& GetRootNode() { return *_root; }
  const SceneNode& GetRootNode() const { return *_root; }
  bool IsSceneDirty() const { return _isSceneDirty; }
  void MarkSceneDirty() { _isSceneDirty = true; }
  size_t RequestUniqueId();
  AssetManager& GetAssetManager() const { return *_asset; }

 private:
  void OnUpdate();

  Unique<UIManager> _ui;
  Share<spdlog::logger> _logger;

  bool _isWorkspaceActive{false};
  std::string _sceneName;
  Unique<FactoryManager> _factory;
  Unique<AssetManager> _asset;
  Unique<SceneNode> _root;
  bool _isSceneDirty{false};
  size_t _nowId{0};
};

}  // namespace Rad
