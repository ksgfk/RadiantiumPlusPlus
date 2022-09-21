#pragma once

#include "../common.h"
#include "build_context.h"

#include <nlohmann/json.hpp>

#include <istream>
#include <filesystem>
#include <optional>
#include <type_traits>
#include <vector>

namespace Rad {

using JsonType = nlohmann::detail::value_t;

template <typename T>  //类模板偏特化
struct IsDefaultConfigData : std::false_type {};
template <>
struct IsDefaultConfigData<Float> : std::true_type {};
template <>
struct IsDefaultConfigData<UInt32> : std::true_type {};
template <>
struct IsDefaultConfigData<Int32> : std::true_type {};
template <>
struct IsDefaultConfigData<std::string> : std::true_type {};

template <typename T>
struct IsVecConfigData : std::false_type {};
template <>
struct IsVecConfigData<Vector2> : std::true_type {};
template <>
struct IsVecConfigData<Vector3> : std::true_type {};
template <>
struct IsVecConfigData<Vector4> : std::true_type {};
template <>
struct IsVecConfigData<Color> : std::true_type {};
template <>
struct IsVecConfigData<Eigen::Vector2i> : std::true_type {};

template <typename T>
struct IsMatConfigData : std::false_type {};
template <>
struct IsMatConfigData<Matrix3> : std::true_type {};
template <>
struct IsMatConfigData<Matrix4> : std::true_type {};

template <JsonType J>
struct ConfigDataType : std::integral_constant<JsonType, J> {};
using NullDataType = ConfigDataType<JsonType::null>;  // Type Aliases
using FloatDataType = ConfigDataType<JsonType::number_float>;
using UnsignedIntDataType = ConfigDataType<JsonType::number_unsigned>;
using IntDataType = ConfigDataType<JsonType::number_integer>;
using StringDataType = ConfigDataType<JsonType::string>;
using ArrayDataType = ConfigDataType<JsonType::array>;
using ObjectDataType = ConfigDataType<JsonType::object>;

template <typename T>
struct NodeType : NullDataType {};
template <>
struct NodeType<Float> : FloatDataType {};
template <>
struct NodeType<UInt32> : UnsignedIntDataType {};
template <>
struct NodeType<Int32> : IntDataType {};
template <>
struct NodeType<std::string> : StringDataType {};
template <>
struct NodeType<Vector2> : ArrayDataType {};
template <>
struct NodeType<Eigen::Vector2i> : ArrayDataType {};
template <>
struct NodeType<Vector3> : ArrayDataType {};
template <>
struct NodeType<Vector4> : ArrayDataType {};
template <>
struct NodeType<Color> : ArrayDataType {};
template <>
struct NodeType<Matrix3> : ObjectDataType {};
template <>
struct NodeType<Matrix4> : ObjectDataType {};

/**
 * @brief 本质上是 nlohmann::json 的 view, 需要确保 json 生命周期比view长
 *
 */
struct ConfigNode {
  ConfigNode() = default;
  inline ConfigNode(const nlohmann::json* d) : data(d) {}

  inline bool HasNode(const std::string& name) const {
    return data->find(name) != data->end();
  }

  template <typename T>
  static bool CheckNodeType(const nlohmann::json& j, JsonType type) {
    if (j.type() == type) {
      return true;
    }
    if (j.is_number() &&
        (type == JsonType::number_float ||
         type == JsonType::number_integer ||
         type == JsonType::number_unsigned)) {
      return true;
    }
    if (j.is_number() && type == JsonType::array && IsVecConfigData<T>::value) {
      return true;
    }
    return false;
  }

  template <typename T>
  static T ReadValue(const nlohmann::json& j) {  // 你不应该尝试读懂这里写了啥 :)
    if (!CheckNodeType<T>(j, NodeType<T>::value)) {
      throw RadInvalidOperationException(
          "target type is: {}. but config node type is: {}",
          NodeType<T>::value,
          j.type());
    }

    if constexpr (NodeType<T>::value == JsonType::object &&
                  !IsDefaultConfigData<T>::value &&
                  !IsVecConfigData<T>::value &&
                  !IsMatConfigData<T>::value) {
      return ConfigNode(&j);
    } else if constexpr (NodeType<T>::value == JsonType::array &&
                         !IsDefaultConfigData<T>::value &&
                         !IsVecConfigData<T>::value &&
                         !IsMatConfigData<T>::value) {
      std::vector<ConfigNode> res;
      res.reserve(j.size());
      for (const auto& d : j) {
        res.emplace_back(&d);
      }
      return res;
    } else if constexpr (IsDefaultConfigData<T>::value) {
      return j.get<T>();
    } else if constexpr (IsVecConfigData<T>::value) {
      if (j.is_array()) {
        if (j.size() != T::SizeAtCompileTime) {
          throw RadInvalidOperationException("node array size not {}", (UInt32)T::SizeAtCompileTime);
        }
        T v;
        for (size_t i = 0; i < j.size(); i++) {
          v[i] = j[i].get<typename T::Scalar>();
        }
        return v;
      } else if (j.is_number()) {
        typename T::Scalar v = j.get<typename T::Scalar>();
        T result;
        for (size_t i = 0; i < T::SizeAtCompileTime; i++) {
          result[i] = v;
        }
        return result;
      } else {
        throw RadInvalidOperationException("unknown type");
      }
    } else if constexpr (IsMatConfigData<T>::value) {
      if (j.size() != T::SizeAtCompileTime) {
        throw RadInvalidOperationException("node array size not {}", (UInt32)T::SizeAtCompileTime);
      }
      std::array<typename T::Scalar, T::SizeAtCompileTime> arr;
      for (size_t i = 0; i < j.size(); i++) {
        arr[i] = j[i].get<typename T::Scalar>();
      }
      return Eigen::Map<T>(arr.data());
    } else {
      throw RadInvalidOperationException("unknown type");
    }
  }

  template <typename T>
  T As() const {
    if (data == nullptr) {
      throw RadInvalidOperationException("null node");
    }
    return ReadValue<T>(*data);
  }

  template <typename T>
  bool TryRead(const std::string& name, T& value) const {
    if (data == nullptr) {
      return false;
    }
    auto iter = data->find(name);
    if (iter == data->end()) {
      return false;
    }
    value = ReadValue<T>(iter.value());
    return true;
  }

  template <typename T>
  T Read(const std::string& name) const {
    if (data == nullptr) {
      throw RadInvalidOperationException("null node");
    }
    auto iter = data->find(name);
    if (iter == data->end()) {
      throw RadArgumentException("no config node: {}", name);
    }
    return ReadValue<T>(iter.value());
  }

  template <typename T>
  T ReadOrDefault(const std::string& name, const T& defVal) const {
    if (data == nullptr) {
      return defVal;
    }
    auto iter = data->find(name);
    return iter == data->end() ? defVal : ReadValue<T>(iter.value());
  }

  inline Matrix4 AsTransform() const {
    if (data->type() == nlohmann::detail::value_t::array) {
      return As<Matrix4>();
    } else if (data->type() == nlohmann::detail::value_t::object) {
      Vector3 translate = ReadOrDefault("translate", Vector3(0, 0, 0));
      Vector3 scale = ReadOrDefault("scale", Vector3(1, 1, 1));
      Eigen::AngleAxis<Float> rotation = Eigen::AngleAxis<Float>::Identity();
      ConfigNode rotateNode;
      if (TryRead("rotate", rotateNode)) {
        Vector3 axis = ReadOrDefault("axis", Vector3(0, 1, 0));
        Float angle = ReadOrDefault("angle", Float(0));
        rotation = Eigen::AngleAxis<Float>(Math::Radian(angle), axis);
      }
      Eigen::Translation<Float, 3> t(translate);
      Eigen::DiagonalMatrix<Float, 3> s(scale);
      Eigen::Transform<Float, 3, Eigen::Affine> affine = t * rotation * s;
      return affine.matrix();
    } else {
      throw RadInvalidOperationException("transform node must be mat4 or object");
    }
  }

  template <typename T>
  Share<Texture<T>> ReadTexture(
      BuildContext& ctx,
      const std::string& name,
      const T& defVal) const {
    if (data == nullptr) {
      return std::make_shared<Texture<T>>(defVal);
    }
    auto iter = data->find(name);
    if (iter == data->end()) {
      return std::make_shared<Texture<T>>(defVal);
    }
    ConfigNode tex(&iter.value());
    if (tex.data->is_number() || tex.data->is_array()) {
      T value = tex.As<T>();
      return std::make_shared<Texture<T>>(value);
    }
    std::string type = tex.Read<std::string>("type");
    TextureBaseFactory* factory = ctx.GetFactory<TextureBaseFactory>(type);
    Unique<TextureBase> texInstance = factory->Create(&ctx, tex);
    Share<TextureBase> share = std::move(texInstance);
#if defined(RAD_DEFINE_DEBUG)
    Share<Texture<T>> result = std::dynamic_pointer_cast<Texture<T>, TextureBase>(share);
#else
    Share<Texture<T>> result = std::static_pointer_cast<Texture<T>, TextureBase>(share);
#endif
    return result;
  }

  const nlohmann::json* data = nullptr;
};

template <>
struct NodeType<ConfigNode> : ObjectDataType {};
template <>
struct NodeType<std::vector<ConfigNode>> : ArrayDataType {};

}  // namespace Rad

std::ostream& operator<<(std::ostream& os, nlohmann::detail::value_t type);
template <>
struct spdlog::fmt_lib::formatter<nlohmann::detail::value_t> : spdlog::fmt_lib::ostream_formatter {};
