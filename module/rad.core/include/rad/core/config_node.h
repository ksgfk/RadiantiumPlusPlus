#pragma once

#include "types.h"
#include "color.h"

#include <nlohmann/json.hpp>

#include <string>

namespace Rad {

template <typename T>
T JsonToVector(const nlohmann::json& data) {
  T result;
  if (data.is_array()) {
    if (data.size() != T::SizeAtCompileTime) {
      throw RadArgumentException("node array size must be {}", (UInt32)T::SizeAtCompileTime);
    }
    for (const auto& i : data) {
      if (!i.is_number()) {
        throw RadArgumentException("node array can only contain numbers");
      }
    }
    for (size_t i = 0; i < data.size(); i++) {
      result[i] = data[i].get<typename T::Scalar>();
    }
  } else if (data.is_number()) {
    typename T::Scalar v = data.get<typename T::Scalar>();
    result = T::Constant(v);
  } else {
    throw RadArgumentException("node must be {} or {}", "array", "number");
  }
  return result;
}

template <typename T>
T JsonToMatrix(const nlohmann::json& data) {
  if (data.size() != T::SizeAtCompileTime) {
    throw RadArgumentException("node array size must be {}", (UInt32)T::SizeAtCompileTime);
  }
  for (const auto& i : data) {
    if (!i.is_number()) {
      throw RadArgumentException("node array can only contain numbers");
    }
  }
  T mat;
  int k = 0;
  for (int i = 0; i < T::MaxRowsAtCompileTime; i++) {
    for (int j = 0; j < T::MaxColsAtCompileTime; j++) {
      mat(i, j) = data[k].get<typename T::Scalar>();
      k++;
    }
  }
  return mat;
}

class RAD_EXPORT_API ConfigNode {
 public:
  ConfigNode() = default;
  inline ConfigNode(const nlohmann::json* data) : _data(data) {}
  ~ConfigNode() noexcept = default;

  const nlohmann::json* GetData() const { return _data; }

  inline bool HasNode(const std::string& name) const {
    return _data->find(name) != _data->end();
  }

  template <typename T>
  T As() const {
    if constexpr (std::is_same_v<T, bool>) {
      if (!_data->is_boolean()) throw RadArgumentException("node type is not {}", "bool");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, Float32>) {
      if (!_data->is_number()) throw RadArgumentException("node type is not {}", "Float32");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, Float64>) {
      if (!_data->is_number()) throw RadArgumentException("node type is not {}", "Float64");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, UInt32>) {
      if (!_data->is_number()) throw RadArgumentException("node type is not {}", "UInt32");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, Int32>) {
      if (!_data->is_number()) throw RadArgumentException("node type is not {}", "Int32");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, std::string>) {
      if (!_data->is_string()) throw RadArgumentException("node type is not {}", "string");
      return _data->get<T>();
    } else if constexpr (std::is_same_v<T, Vector2f>) {
      return JsonToVector<Vector2f>(*_data);
    } else if constexpr (std::is_same_v<T, Vector3f>) {
      return JsonToVector<Vector3f>(*_data);
    } else if constexpr (std::is_same_v<T, Vector4f>) {
      return JsonToVector<Vector4f>(*_data);
    } else if constexpr (std::is_same_v<T, Color24f>) {
      return JsonToVector<Color24f>(*_data);
    } else if constexpr (std::is_same_v<T, Vector2i>) {
      return JsonToVector<Vector2i>(*_data);
    } else if constexpr (std::is_same_v<T, Matrix3f>) {
      return JsonToMatrix<Matrix3f>(*_data);
    } else if constexpr (std::is_same_v<T, Matrix4f>) {
      return JsonToMatrix<Matrix4f>(*_data);
    } else if constexpr (std::is_same_v<T, ConfigNode>) {
      if (!_data->is_object() && !_data->is_array()) throw RadArgumentException("node type is not {}", "object or array");
      return *this;
    } else if constexpr (std::is_same_v<T, std::vector<ConfigNode>>) {
      if (!_data->is_array()) throw RadArgumentException("node type is not {}", "array");
      std::vector<ConfigNode> result;
      for (const auto& i : *_data) {
        result.emplace_back(ConfigNode(&i));
      }
      return result;
    } else {
      static_assert(std::is_same_v<T, void>, "unsupported type");
    }
  }

  template <typename T>
  T ReadOrDefault(const std::string& name, const T& defVal) const {
    if (_data == nullptr) {
      return defVal;
    }
    auto iter = _data->find(name);
    if (iter == _data->end()) {
      return defVal;
    }
    ConfigNode child(&iter.value());
    return child.As<T>();
  }

  template <typename T>
  T Read(const std::string& name) const {
    if (_data == nullptr) {
      throw RadArgumentException("node is null");
    }
    auto iter = _data->find(name);
    if (iter == _data->end()) {
      throw RadArgumentException("no child: {}", name);
    }
    ConfigNode child(&iter.value());
    return child.As<T>();
  }

  template <typename T>
  bool TryRead(const std::string& name, T& value) const {
    if (_data == nullptr) {
      return false;
    }
    auto iter = _data->find(name);
    if (iter == _data->end()) {
      return false;
    }
    ConfigNode child(&iter.value());
    value = child.As<T>();
    return true;
  }

 private:
  const nlohmann::json* _data{nullptr};
};

}  // namespace Rad
