#pragma once

#include <rad/core/config_node.h>
#include <rad/offline/types.h>
#include <rad/offline/render/texture.h>
#include "build_context.h"
#include "factory.h"

//一些渲染器特有的配置参数，这里放一些读取配置参数的帮助函数

namespace Rad {

Matrix3 ConfigNodeAsRotate(ConfigNode node);
Matrix3 ConfigNodeReadOrDefaultRotate(ConfigNode node, const std::string& name, const Matrix3& defVal);
bool ConfigNodeTryReadRotate(ConfigNode node, const std::string& name, Matrix3& result);
template <typename T>
Unique<Texture<T>> ConfigNodeReadTexture(
    BuildContext* ctx,
    ConfigNode cfg,
    const std::string& name,
    const T& defVal) {
  static_assert(std::is_same_v<T, Color24f> || std::is_same_v<T, Float32>, "T must be Color or Float32");
  if (cfg.GetData() == nullptr) {
    return std::make_unique<Texture<T>>(defVal);
  }
  auto iter = cfg.GetData()->find(name);
  if (iter == cfg.GetData()->end()) {
    return std::make_unique<Texture<T>>(defVal);
  }
  ConfigNode tex(&iter.value());
  if (tex.GetData()->is_number() || tex.GetData()->is_array()) {
    T value = tex.As<T>();
    return std::make_unique<Texture<T>>(value);
  }
  std::string type = tex.Read<std::string>("type");
  TextureFactory* factory = ctx->GetFactoryManager().GetFactory<TextureFactory>(type);
  Unique<TextureBase> texInstance;
  if constexpr (std::is_same_v<T, Color24f>) {
    return factory->CreateRgb(ctx, tex);
  } else if constexpr (std::is_same_v<T, Float32>) {
    return factory->CreateMono(ctx, tex);
  } else {
    return nullptr;
  }
}
Matrix4 ConfigNodeAsTransform(ConfigNode node);
Unique<Volume> ConfigNodeReadVolume(
    BuildContext* ctx,
    ConfigNode cfg,
    const std::string& name,
    const Spectrum& defVal);

}  // namespace Rad
