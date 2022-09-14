#include <radiantium/config_node.h>

#include <radiantium/math_ext.h>
#include <radiantium/build_context.h>
#include <radiantium/factory.h>
#include <radiantium/texture.h>

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <fmt/core.h>

using json = nlohmann::json;

namespace rad::config {

class JsonConfigNode : public IConfigNode {
 public:
  JsonConfigNode() = default;
  JsonConfigNode(json&& json) : _j(std::move(json)) {}
  JsonConfigNode(const json&& json) : _j(std::move(json)) {}
  ~JsonConfigNode() noexcept override {}

  static Type CvtType(json::value_t type) {
    switch (type) {
      case nlohmann::detail::value_t::null:
        return Type::Empty;
      case nlohmann::detail::value_t::object:
        return Type::Object;
      case nlohmann::detail::value_t::array:
        return Type::Array;
      case nlohmann::detail::value_t::string:
        return Type::String;
      case nlohmann::detail::value_t::boolean:
        return Type::Bool;
      case nlohmann::detail::value_t::number_integer:
        return Type::Int;
      case nlohmann::detail::value_t::number_unsigned:
        return Type::Int;
      case nlohmann::detail::value_t::number_float:
        return Type::Float;
      case nlohmann::detail::value_t::binary:
        return Type::Unknown;
      case nlohmann::detail::value_t::discarded:
        return Type::Unknown;
      default:
        return Type::Unknown;
    }
  }

  Type GetType() const override {
    auto type = _j.type();
    return CvtType(type);
  }

  Type GetChildType(const std::string& name) const override {
    return CvtType(_j.find(name)->type());
  }

  bool HasProperty(const std::string& name) const override {
    return _j.find(name) != _j.end();
  }

  template <typename T>
  static T Get(const json& node) { return node.get<T>(); }
  template <typename T>
  T Get(const std::string& name) const {
    auto iter = _j.find(name);
    return Get<T>(*iter);
  }
  template <typename T>
  T Get(const std::string& name, const T& defVal) const {
    auto iter = _j.find(name);
    return iter == _j.end() ? defVal : Get<T>(*iter);
  }

  bool GetBool(const std::string& name) const override { return Get<bool>(name); }
  bool GetBool(const std::string& name, bool defVal) const override { return Get<bool>(name, defVal); }

  Int32 GetInt32(const std::string& name) const override { return Get<Int32>(name); }
  Int32 GetInt32(const std::string& name, Int32 defVal) const override { return Get<Int32>(name, defVal); }

  Float GetFloat(const std::string& name) const override { return Get<Float>(name); }
  Float GetFloat(const std::string& name, Float defVal) const override { return Get<Float>(name, defVal); }

  template <typename T, size_t Count>
  static Eigen::Matrix<T, Count, 1> GetVec(const json& node) {
    Eigen::Matrix<T, Count, 1> v;
    if (node.is_number()) {
      T val = node.get<T>();
      for (size_t i = 0; i < Count; i++) {
        v[i] = val;
      }
    } else {
      if (node.size() != Count) {
        throw std::invalid_argument(fmt::format("invalid vec{}, {}", Count, node.dump()));
      }
      for (size_t i = 0; i < Count; i++) {
        v[i] = node[i].get<T>();
      }
    }
    return v;
  }
  template <typename T, size_t Count>
  Eigen::Vector<T, Count> GetVec(const std::string& name) const { return GetVec<T, Count>(*_j.find(name)); }
  template <typename T, size_t Count>
  Eigen::Vector<T, Count> GetVec(const std::string& name, const Eigen::Vector<T, Count>& defVal) const {
    auto iter = _j.find(name);
    if (iter == _j.end()) {
      return defVal;
    }
    return GetVec<T, Count>(*iter);
  }

  Vec2 GetVec2(const std::string& name) const override { return GetVec<Float, 2>(name); }
  Vec2 GetVec2(const std::string& name, const Vec2& defVal) const override { return GetVec<Float, 2>(name, defVal); }
  Vec3 GetVec3(const std::string& name) const override { return GetVec<Float, 3>(name); }
  Vec3 GetVec3(const std::string& name, const Vec3& defVal) const override { return GetVec<Float, 3>(name, defVal); }
  Vec4 GetVec4(const std::string& name) const override { return GetVec<Float, 4>(name); }
  Vec4 GetVec4(const std::string& name, const Vec4& defVal) const override { return GetVec<Float, 4>(name, defVal); }

  template <size_t Count>
  static Mat4 GetMatImpl(const json& node) {
    Eigen::Matrix<Float, Count, Count> mat;
    if (node.size() != Count * Count) {
      throw std::invalid_argument(fmt::format("invalid mat{}, {}", Count, node.dump()));
    }
    for (size_t j = 0; j < Count; j++) {
      size_t m = j * Count;
      for (size_t i = 0; i < Count; i++) {
        mat(i, j) = node[m + i].get<Float>();
      }
    }
    return mat;
  }
  Mat4 GetMat4(const std::string& name) const override { return GetMatImpl<4>(*_j.find(name)); }
  Mat4 GetMat4(const std::string& name, const Mat4& defVal) const override {
    auto iter = _j.find(name);
    if (iter == _j.end()) {
      return defVal;
    }
    return GetMatImpl<4>(*iter);
  }

  std::string GetString(const std::string& name) const override { return Get<std::string>(name); }
  std::string GetString(const std::string& name, const std::string& defVal) const override {
    return Get<std::string>(name, defVal);
  }

  std::unique_ptr<IConfigNode> GetObject(const std::string& name) const override {
    auto iter = _j.find(name);
    return std::make_unique<JsonConfigNode>(std::move(*iter));
  }
  bool TryGetObject(const std::string& name, std::unique_ptr<IConfigNode>& node) const override {
    auto iter = _j.find(name);
    if (iter == _j.end()) {
      return false;
    }
    node = std::make_unique<JsonConfigNode>(std::move(*iter));
    return true;
  }

  std::vector<std::unique_ptr<IConfigNode>> GetArray(const std::string& name) const override {
    auto iter = _j.find(name);
    std::vector<std::unique_ptr<IConfigNode>> arr(iter->size());
    for (size_t i = 0; i < iter->size(); i++) {
      arr[i] = std::make_unique<JsonConfigNode>(std::move((*iter)[i]));
    }
    return arr;
  }
  bool TryGetArray(const std::string& name, std::vector<std::unique_ptr<IConfigNode>>& array) const override {
    auto iter = _j.find(name);
    if (iter == _j.end()) {
      return false;
    }
    array = std::vector<std::unique_ptr<IConfigNode>>(iter->size());
    for (size_t i = 0; i < iter->size(); i++) {
      array[i] = std::make_unique<JsonConfigNode>(std::move((*iter)[i]));
    }
    return true;
  }

 private:
  json _j;
};

std::unique_ptr<IConfigNode> CreateJsonConfig(const std::filesystem::path& path) {
  std::ifstream stream(path);
  json j = json::parse(stream);
  return std::make_unique<JsonConfigNode>(std::move(j));
}

std::unique_ptr<IConfigNode> CreateJsonConfig(const std::string& config) {
  json j = json::parse(config);
  return std::make_unique<JsonConfigNode>(std::move(j));
}

}  // namespace rad::config

namespace rad {

const IConfigNode* IConfigNode::Empty() {
  static std::unique_ptr<IConfigNode> empty = rad::config::CreateJsonConfig(std::string("{}"));
  return empty.get();
}

std::unique_ptr<IConfigNode> IConfigNode::CreateEmpty() {
  return rad::config::CreateJsonConfig(std::string("{}"));
}

static Mat4 ToTransform(const IConfigNode* node) {
  Vec3 translate = node->GetVec3("translate", Vec3(0, 0, 0));
  Vec3 scale = node->GetVec3("scale", Vec3(1, 1, 1));
  Mat4 rotation = Mat4::Identity();
  if (node->HasProperty("rotate")) {
    auto rotateNode = node->GetObject("rotate");
    Vec3 axis = rotateNode->GetVec3("axis", Vec3(0, 1, 0));
    Float angle = rotateNode->GetFloat("angle", Float(0));
    Eigen::AngleAxis<Float> angleAxis(math::Radian(angle), axis);
    rotation.bottomLeftCorner<3, 3>() = angleAxis.toRotationMatrix();
  }
  Mat4 t;
  t << 1, 0, 0, translate.x(),
      0, 1, 0, translate.y(),
      0, 0, 1, translate.z(),
      0, 0, 0, 1;
  Mat4 s;
  s << scale.x(), 0, 0, 0,
      0, scale.y(), 0, 0,
      0, 0, scale.z(), 0,
      0, 0, 0, 1;
  return t * rotation * s;
}
Mat4 IConfigNode::GetTransform(const IConfigNode* node, const std::string& name) {
  auto n = node->GetObject(name);
  return ToTransform(n.get());
}
Mat4 IConfigNode::GetTransform(
    const IConfigNode* node,
    const std::string& name,
    const Mat4& transform) {
  if (!node->HasProperty(name)) {
    return transform;
  }
  auto n = node->GetObject(name);
  return ToTransform(n.get());
}

std::unique_ptr<ITexture> IConfigNode::GetTexture(
    const IConfigNode* node,
    const BuildContext* ctx,
    const std::string& name,
    const RgbSpectrum& defVal) {
  if (!node->HasProperty(name)) {
    auto json = fmt::format("{{\"value\":[{},{},{}]}}", defVal.R(), defVal.G(), defVal.B());
    auto node = config::CreateJsonConfig(json);
    return ctx->GetFactory<ITextureFactory>("texture_const")->Create(ctx, node.get());
  }
  if (node->GetChildType(name) == IConfigNode::Type::Array) {
    Vec3 cst = node->GetVec3(name);
    auto json = fmt::format("{{\"value\":[{},{},{}]}}", cst.x(), cst.y(), cst.z());
    auto node = config::CreateJsonConfig(json);
    return ctx->GetFactory<ITextureFactory>("texture_const")->Create(ctx, node.get());
  } else {
    auto texNode = node->GetObject(name);
    std::string type = texNode->GetString("type");
    auto factory = ctx->GetFactory<ITextureFactory>(type);
    return factory->Create(ctx, texNode.get());
  }
}

}  // namespace rad
