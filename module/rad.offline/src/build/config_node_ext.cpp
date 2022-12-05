#include <rad/offline/build/config_node_ext.h>

#include <rad/offline/render/volume.h>

namespace Rad {

Matrix3 ConfigNodeAsRotate(ConfigNode node) {
  Matrix3 result;
  if (node.GetData()->is_object()) {
    Vector3 axis = node.ReadOrDefault("axis", Vector3(0, 1, 0));
    Float angle = node.ReadOrDefault("angle", Float(0));
    Eigen::AngleAxis<Float> rotation(Math::Radian(angle), axis);
    result = rotation.toRotationMatrix();
  } else if (node.GetData()->is_array()) {
    if (node.GetData()->size() == 9) {
      result = node.As<Matrix3>();
    } else if (node.GetData()->size() == 4) {
      Vector4 vec4 = node.As<Vector4>();
      Eigen::Quaternion<Float> q(vec4);
      result = q.toRotationMatrix();
    } else {
      throw RadArgumentException("rotate array must be mat3 or quaternion");
    }
  } else {
    throw RadArgumentException("rotate node must be array or object");
  }
  return result;
}

Matrix3 ConfigNodeReadOrDefaultRotate(
    ConfigNode node,
    const std::string& name,
    const Matrix3& defVal) {
  auto iter = node.GetData()->find(name);
  if (iter == node.GetData()->end()) {
    return defVal;
  }
  ConfigNode child(&iter.value());
  return ConfigNodeAsRotate(child);
}

bool ConfigNodeTryReadRotate(ConfigNode node, const std::string& name, Matrix3& result) {
  auto iter = node.GetData()->find(name);
  if (iter == node.GetData()->end()) {
    return false;
  }
  ConfigNode child(&iter.value());
  result = ConfigNodeAsRotate(child);
  return true;
}

Matrix4 ConfigNodeAsTransform(ConfigNode node) {
  Matrix4 result;
  if (node.GetData()->is_array()) {
    result = node.As<Matrix4>();
  } else if (node.GetData()->is_object()) {
    Vector3 translate = node.ReadOrDefault("translate", Vector3(0, 0, 0));
    Vector3 scale = node.ReadOrDefault("scale", Vector3(1, 1, 1));
    Matrix3 rotation;
    if (!ConfigNodeTryReadRotate(node, "rotate", rotation)) {
      rotation = Matrix3::Identity();
    }
    Eigen::Translation<Float, 3> t(translate);
    Eigen::DiagonalMatrix<Float, 3> s(scale);
    Eigen::Transform<Float, 3, Eigen::Affine> affine = t * rotation * s;
    result = affine.matrix();
  } else {
    throw RadArgumentException("transform node must be mat4 or object");
  }
  return result;
}

Unique<Volume> ConfigNodeReadVolume(
    BuildContext* ctx,
    ConfigNode cfg,
    const std::string& name,
    const Spectrum& defVal) {
  if (cfg.GetData() == nullptr) {
    return std::make_unique<Volume>(defVal);
  }
  auto iter = cfg.GetData()->find(name);
  if (iter == cfg.GetData()->end()) {
    return std::make_unique<Volume>(defVal);
  }
  ConfigNode tex(&iter.value());
  if (tex.GetData()->is_number() || tex.GetData()->is_array()) {
    Color24f value = tex.As<Color24f>();
    return std::make_unique<Volume>(Color24fToSpectrum(value));
  }
  std::string type = tex.Read<std::string>("type");
  VolumeFactory* factory = ctx->GetFactoryManager().GetFactory<VolumeFactory>(type);
  Unique<Volume> instance = factory->Create(ctx, tex);
  return instance;
}

}  // namespace Rad
