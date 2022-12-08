#include <rad/offline/editor/editor_application.h>

#include <rad/realtime/window.h>
#include <rad/realtime/opengl_context.h>

#include <thread>
#include <queue>

namespace Rad {

Matrix3f ConfigNodeAsRotatef(ConfigNode node) {
  Matrix3f result;
  if (node.GetData()->is_object()) {
    Vector3f axis = node.ReadOrDefault("axis", Vector3f(0, 1, 0));
    Float32 angle = node.ReadOrDefault("angle", Float32(0));
    Eigen::AngleAxis<Float32> rotation(Math::Radian(angle), axis);
    result = rotation.toRotationMatrix();
  } else if (node.GetData()->is_array()) {
    if (node.GetData()->size() == 9) {
      result = node.As<Matrix3f>();
    } else if (node.GetData()->size() == 4) {
      Vector4f vec4 = node.As<Vector4f>();
      Eigen::Quaternion<Float32> q(vec4);
      result = q.toRotationMatrix();
    } else {
      throw RadArgumentException("rotate array must be mat3 or quaternion");
    }
  } else {
    throw RadArgumentException("rotate node must be array or object");
  }
  return result;
}

bool ConfigNodeTryReadRotatef(ConfigNode node, const std::string& name, Matrix3f& result) {
  auto iter = node.GetData()->find(name);
  if (iter == node.GetData()->end()) {
    return false;
  }
  ConfigNode child(&iter.value());
  result = ConfigNodeAsRotatef(child);
  return true;
}

Matrix4f ConfigNodeAsTransformf(ConfigNode node) {
  Matrix4f result;
  if (node.GetData()->is_array()) {
    result = node.As<Matrix4f>();
  } else if (node.GetData()->is_object()) {
    Vector3f translate = node.ReadOrDefault("translate", Vector3f(0, 0, 0));
    Vector3f scale = node.ReadOrDefault("scale", Vector3f(1, 1, 1));
    Matrix3f rotation;
    if (!ConfigNodeTryReadRotatef(node, "rotate", rotation)) {
      rotation = Matrix3f::Identity();
    }
    Eigen::Translation<Float32, 3> t(translate);
    Eigen::DiagonalMatrix<Float32, 3> s(scale);
    Eigen::Transform<Float32, 3, Eigen::Affine> affine = t * rotation * s;
    result = affine.matrix();
  } else {
    throw RadArgumentException("transform node must be mat4 or object");
  }
  return result;
}

void SceneNode::AfterAddChild(std::list<SceneNode>::iterator& childIter) {
  auto&& child = *childIter;
  child._parent = this;
  child._inParentPos = childIter;
  _editor->MarkSceneDirty();
}

SceneNode::SceneNode(EditorApplication* editor) {
  _editor = editor;
  _uniqueId = _editor->RequestUniqueId();
  _toWorld = Matrix4f::Identity();
  _toLocal = Matrix4f::Identity();
  _localToWorld = Matrix4f::Identity();
  _parent = nullptr;
}

SceneNode& SceneNode::AddChildBefore(const SceneNode& pos, SceneNode&& node) {
  if (node._parent != nullptr) {
    node._parent->RemoveChild(node);
  }
  auto childIter = _children.emplace(pos._inParentPos, std::move(node));
  AfterAddChild(childIter);
  return *childIter;
}

SceneNode& SceneNode::AddChildAfter(const SceneNode& pos, SceneNode&& node) {
  if (node._parent != nullptr) {
    node._parent->RemoveChild(node);
  }
  auto iter = pos._inParentPos;
  auto childIter = _children.emplace(iter++, std::move(node));
  AfterAddChild(childIter);
  return *childIter;
}

SceneNode& SceneNode::AddChildLast(SceneNode&& node) {
  if (node._parent != nullptr) {
    node._parent->RemoveChild(node);
  }
  auto childIter = _children.emplace(_children.end(), std::move(node));
  AfterAddChild(childIter);
  return *childIter;
}

void SceneNode::RemoveChild(SceneNode& child) {
  if (child._parent != this) {
    throw RadArgumentException("parent and child not same!");
  }
  _children.erase(child._inParentPos);
  child._parent = nullptr;
  child._inParentPos = std::list<SceneNode>::iterator{};
}

void SceneNode::SetToWorldMatrix(const Matrix4f& toWorld) {
  _localToWorld = toWorld;
  _editor->MarkSceneDirty();
}

EditorApplication::EditorApplication(int argc, char** argv) {
  _logger = Logger::GetCategory("Editor");
  _factory = std::make_unique<FactoryManager>();
  _asset = std::make_unique<AssetManager>();
  _asset->SetObjectFactory(*_factory.get());
  for (auto&& i : GetRadCoreAssetFactories()) {
    _factory->RegisterFactory(i());
  }
  RadInitGlfw();
  Rad::GlfwWindowOptions opts{};
  opts.Title = "Rad-Offline Editor v0.0.1";
  opts.EnableOpenGL = true;
  opts.EnableOpenGLDebugLayer = true;
  opts.IsMaximize = true;
  RadCreateWindowGlfw(opts);
  RadShowWindowGlfw();
  RadInitContextOpenGL();
  _ui = std::make_unique<UIManager>(this);
}

EditorApplication::~EditorApplication() noexcept {
  _ui = nullptr;
  RadShutdownContextOpenGL();
  RadDestroyWindowGlfw();
  RadShutdownGlfw();
}

void EditorApplication::Run() {
  while (!Rad::RadShouldCloseWindowGlfw()) {
    Rad::RadPollEventGlfw();
    OnUpdate();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _ui->NewFrame();
    _ui->OnGui();
    _ui->Draw();
    Rad::RadSwapBuffersGlfw();
    std::this_thread::yield();
  }
}

void EditorApplication::OpenWorkspace(const std::filesystem::path& filePath) {
  _logger->info("start open workspace: {}", filePath.u8string());
  if (!std::filesystem::exists(filePath)) {
    _logger->error("cannot open workspace: {}", filePath.u8string());
  }
  nlohmann::json cfg;
  {
    std::ifstream cfgStream(filePath);
    if (!cfgStream.is_open()) {
      _logger->error("cannot open workspace: {}", filePath.u8string());
      return;
    }
    try {
      cfg = nlohmann::json::parse(cfgStream);
    } catch (const std::exception& e) {
      _logger->error("cannot open workspace: {}", e.what());
      return;
    }
  }
  if (IsWorkspaceActive()) {
    // TODO: 提示保存当前场景，清理现场
    _isWorkspaceActive = false;
    _root = SceneNode{this};
  }
  ConfigNode root(&cfg);
  // 拿到配置文件了，首先加载资产
  // _asset->SetWorkDirectory(filePath.parent_path());
  // _asset->SetImageStorageIsBlockBased(false);
  // std::vector<ConfigNode> assetNode;
  // if (root.TryRead("assets", assetNode)) {
  //   for (ConfigNode node : assetNode) {
  //     AssetLoadResult result = _asset->Load(node);
  //     if (!result.IsSuccess) {
  //       _logger->error("cannot open workspace: {}", result.FailReason);
  //       return;
  //     }
  //   }
  // }
  // 然后构造场景的树状结构
  auto that = this;
  auto createNewNode = [=](ConfigNode i) {
    SceneNode newNode{that};
    newNode.Name = "SceneObject";
    newNode.Name = i.ReadOrDefault("name", newNode.Name);
    Matrix4f toWorld = Matrix4f::Identity();
    ConfigNode toWorldNode;
    if (i.TryRead("to_world", toWorldNode)) {
      toWorld = ConfigNodeAsTransformf(toWorldNode);
    }
    newNode.SetToWorldMatrix(toWorld);
    return newNode;
  };
  {
    std::vector<ConfigNode> sceneNode;
    std::vector<ConfigNode> childrenNodes;
    std::queue<std::pair<SceneNode*, ConfigNode>> q;
    if (root.TryRead("scene", sceneNode)) {
      for (auto i : sceneNode) {
        childrenNodes.clear();
        SceneNode newNode = createNewNode(i);
        SceneNode& thisNode = _root.AddChildLast(std::move(newNode));
        if (i.TryRead("children", childrenNodes)) {
          for (auto j : childrenNodes) {
            q.emplace(std::make_pair(&thisNode, j));
          }
        }
      }
    }
    while (!q.empty()) {
      childrenNodes.clear();
      auto [parent, childConfig] = q.front();
      q.pop();
      SceneNode newNode = createNewNode(childConfig);
      SceneNode& thisNode = parent->AddChildLast(std::move(newNode));
      if (childConfig.TryRead("children", childrenNodes)) {
        for (auto j : childrenNodes) {
          q.emplace(std::make_pair(&thisNode, j));
        }
      }
    }
  }
  _sceneName = filePath.filename().replace_extension().u8string();
  _root.Name = _sceneName;
  _isWorkspaceActive = true;
  _logger->info("done");
}

bool EditorApplication::IsWorkspaceActive() const {
  return _isWorkspaceActive;
}

void EditorApplication::OnUpdate() {
}

size_t EditorApplication::RequestUniqueId() {
  _nowId++;
  return _nowId;
}

}  // namespace Rad
