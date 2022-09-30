#include <rad/offline/build/config_node.h>

#include <fstream>
#include <array>

namespace Rad {

bool ConfigNode::TryReadRotate(const std::string& name, Matrix3& result) const {
  auto iter = data->find(name);
  if (iter == data->end()) {
    return false;
  }
  ConfigNode node(&iter.value());
  result = node.AsRotate();
  return true;
}

Matrix3 ConfigNode::AsRotate() const {
  if (data->type() == nlohmann::detail::value_t::object) {  //轴角
    Vector3 axis = ReadOrDefault("axis", Vector3(0, 1, 0));
    Float angle = ReadOrDefault("angle", Float(0));
    Eigen::AngleAxis<Float> rotation(Math::Radian(angle), axis);
    return rotation.toRotationMatrix();
  } else if (data->type() == nlohmann::detail::value_t::array) {
    if (data->size() == 9) {  //直接一个矩阵, 注意是3x3的
      return As<Matrix3>();
    } else if (data->size() == 4) {  //四元数
      Vector4 vec4 = As<Vector4>();
      Eigen::Quaternion<Float> q(vec4);
      return q.toRotationMatrix();
    } else {
      throw RadInvalidOperationException("rotate array must be mat3 or quaternion");
    }
  } else {
    throw RadInvalidOperationException("rotate node must be array or object");
  }
}

Matrix4 ConfigNode::AsTransform() const {
  if (data->type() == nlohmann::detail::value_t::array) {
    return As<Matrix4>();
  } else if (data->type() == nlohmann::detail::value_t::object) {
    Vector3 translate = ReadOrDefault("translate", Vector3(0, 0, 0));
    Vector3 scale = ReadOrDefault("scale", Vector3(1, 1, 1));
    Matrix3 rotation;
    if (!TryReadRotate("rotate", rotation)) {
      rotation = Matrix3::Identity();
    }
    Eigen::Translation<Float, 3> t(translate);
    Eigen::DiagonalMatrix<Float, 3> s(scale);
    Eigen::Transform<Float, 3, Eigen::Affine> affine = t * rotation * s;
    return affine.matrix();
  } else {
    throw RadInvalidOperationException("transform node must be mat4 or object");
  }
}

}  // namespace Rad

std::ostream& operator<<(std::ostream& os, nlohmann::detail::value_t type) {
  switch (type) {
    case nlohmann::detail::value_t::null:
      os << "null";
      break;
    case nlohmann::detail::value_t::object:
      os << "object";
      break;
    case nlohmann::detail::value_t::array:
      os << "array";
      break;
    case nlohmann::detail::value_t::string:
      os << "string";
      break;
    case nlohmann::detail::value_t::boolean:
      os << "bool";
      break;
    case nlohmann::detail::value_t::number_integer:
      os << "int";
      break;
    case nlohmann::detail::value_t::number_unsigned:
      os << "uint";
      break;
    case nlohmann::detail::value_t::number_float:
      os << "float";
      break;
    case nlohmann::detail::value_t::binary:
      os << "binary";
      break;
    case nlohmann::detail::value_t::discarded:
      os << "discarded";
      break;
  }
  return os;
}
