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

std::tuple<Matrix3f, Eigen::Quaternionf, Vector3f> DecomposeTransformf(const Matrix4f& m) {
  Matrix3f Q = m.topLeftCorner<3, 3>();
  for (size_t i = 0; i < 32; i++) {
    Matrix3f Qi = Q.inverse().transpose();
    Float32 gamma = Qi.cwiseProduct(Qi).sum() / Q.cwiseProduct(Q).sum();
    Q = Q * (gamma * 0.5f) + (Qi * ((1 / gamma) * 0.5f));
  }
  Matrix3f A = Q.transpose() * Q;
  if (Q.hasNaN()) {
    Q = Matrix3f::Identity();
  }
  Float32 signQ = Q.determinant();
  if (signQ < 0) Q = -Q;
  if (signQ < 0) A = -A;
  return std::make_tuple(A, Eigen::Quaternionf(Q), m.col(3).head<3>());
}

Vector3f MatToEulerAngleZXY(const Matrix3f& mat) {
  Vector3f result;
  if (mat(1, 2) < 1) {
    if (mat(1, 2) > -1) {
      result.x() = std::asin(mat(1, 2));
      result.z() = std::atan2(-mat(1, 0), mat(1, 1));
      result.y() = std::atan2(-mat(0, 2), mat(2, 2));
    } else {
      result.x() = -Math::PI_F / 2;
      result.z() = -std::atan2(-mat(2, 0), mat(0, 0));
      result.y() = 0;
    }
  } else {
    result.x() = Math::PI_F / 2;
    result.z() = std::atan2(-mat(2, 0), mat(0, 0));
    result.y() = 0;
  }
  return Vector3f(
      Math::Degree(result.x()),
      Math::Degree(result.y()),
      Math::Degree(result.z()));
}

Eigen::Quaternionf EulerAngleToQuatZXY(const Vector3f& eulerAngle) {
  auto [sx, cx] = Math::SinCos(Math::Radian(eulerAngle.x() * 0.5f));
  auto [sy, cy] = Math::SinCos(Math::Radian(eulerAngle.y() * 0.5f));
  auto [sz, cz] = Math::SinCos(Math::Radian(eulerAngle.z() * 0.5f));
  Eigen::Quaternionf q;
  q.w() = cx * cy * cz + sx * sy * sz;
  q.x() = cx * sy * cz + sx * cy * sz;
  q.y() = sx * cy * cz - cx * sy * sz;
  q.z() = -sx * sy * cz + cx * cy * sz;
  return q;
}

Matrix3f EulerAngleToMatZXY(const Vector3f& eulerAngle) {
  auto [sx, cx] = Math::SinCos(Math::Radian(eulerAngle.x()));
  auto [sy, cy] = Math::SinCos(Math::Radian(eulerAngle.y()));
  auto [sz, cz] = Math::SinCos(Math::Radian(eulerAngle.z()));
  Matrix3f result;
  result(0, 0) = cy * cz + sx * sy * sz;
  result(1, 0) = cz * sx * sy - cy * sz;
  result(2, 0) = cx * sy;
  result(0, 1) = cx * sz;
  result(1, 1) = cx * cz;
  result(2, 1) = -sx;
  result(0, 2) = cy * sx * sz - cz * sy;
  result(1, 2) = cy * cz * sx + sy * sz;
  result(2, 2) = cx * cy;
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
  Position = Vector3f::Zero();
  Scale = Vector3f::Constant(1);
  Rotation = Eigen::Quaternionf::Identity();
  _parent = nullptr;
  _isSelect = false;
}

SceneNode& SceneNode::AddChildBefore(const SceneNode& pos, SceneNode&& node) {
  SceneNode temp = std::move(node);
  if (temp._parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto childIter = _children.emplace(pos._inParentPos, std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

SceneNode& SceneNode::AddChildAfter(const SceneNode& pos, SceneNode&& node) {
  SceneNode temp = std::move(node);
  if (temp._parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto iter = pos._inParentPos;
  auto childIter = _children.emplace(++iter, std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

SceneNode& SceneNode::AddChildLast(SceneNode&& node) {
  SceneNode temp = std::move(node);
  if (temp._parent != nullptr) {
    throw Rad::RadInvalidOperationException("node {} cannot have parent", temp.Name);
  }
  auto childIter = _children.emplace(_children.end(), std::move(temp));
  AfterAddChild(childIter);
  return *childIter;
}

void SceneNode::RemoveChild(SceneNode& child) {
  if (child._parent != this) {
    throw RadArgumentException("parent and child not same!");
  }
  _children.erase(child._inParentPos);
  child._inParentPos = {};
  child._parent = nullptr;
}

void SceneNode::SetToWorldMatrix(const Matrix4f& toWorld) {
  _editor->MarkSceneDirty();
}

SceneNode* SceneNode::PreviousNode() {
  auto first = _parent->_children.begin();
  if (_inParentPos == first) {
    return nullptr;
  }
  auto prev = _inParentPos;
  prev--;
  return &(*(prev));
}

SceneNode* SceneNode::NextNode() {
  auto last = GetParent()->_children.end();
  auto next = _inParentPos;
  next++;
  if (next == last) {
    return nullptr;
  }
  return &(*(next));
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
  _root = std::make_unique<SceneNode>(this);
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
    _root = std::make_unique<SceneNode>(this);
  }
  ConfigNode root(&cfg);
  // 拿到配置文件了，首先加载资产
  _asset->SetWorkDirectory(filePath.parent_path());
  _asset->SetImageStorageIsBlockBased(false);
  std::vector<ConfigNode> assetNode;
  if (root.TryRead("assets", assetNode)) {
    for (ConfigNode node : assetNode) {
      AssetLoadResult result = _asset->Load(node);
      if (!result.IsSuccess) {
        _logger->error("cannot open workspace: {}", result.FailReason);
        return;
      }
    }
  }
  // 然后构造场景的树状结构
  auto that = this;
  auto createNewNode = [=](ConfigNode i) {
    SceneNode newNode{that};
    newNode.Name = "SceneObject";
    newNode.Name = i.ReadOrDefault("name", newNode.Name);
    // Matrix4f toWorld = Matrix4f::Identity();
    ConfigNode toWorldNode;
    if (i.TryRead("to_world", toWorldNode)) {
      // toWorld = ConfigNodeAsTransformf(toWorldNode);
      if (toWorldNode.GetData()->is_array()) {
        // TODO: 解压矩阵
        Matrix4f mat = toWorldNode.As<Matrix4f>();
        auto [s, q, t] = DecomposeTransformf(mat);
        newNode.Position = t;
        newNode.Scale = Vector3f(s(0, 0), s(1, 1), s(2, 2));
        newNode.Rotation = q;
      } else if (toWorldNode.GetData()->is_object()) {
        Vector3f translate = toWorldNode.ReadOrDefault("translate", Vector3f(0, 0, 0));
        Vector3f scale = toWorldNode.ReadOrDefault("scale", Vector3f(1, 1, 1));
        Matrix3f rotation;
        if (!ConfigNodeTryReadRotatef(toWorldNode, "rotate", rotation)) {
          rotation = Matrix3f::Identity();
        }
        newNode.Position = translate;
        newNode.Scale = scale;
        newNode.Rotation = Eigen::Quaternionf(rotation);
      }
    }
    // newNode.SetToWorldMatrix(toWorld);
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
        SceneNode& thisNode = _root->AddChildLast(std::move(newNode));
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
  _root->Name = _sceneName;
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
