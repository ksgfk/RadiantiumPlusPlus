#pragma once

#include <rad/core/asset.h>
#include <rad/core/factory.h>

#include <rad/offline/fwd.h>
#include <rad/offline/types.h>

namespace Rad {

class RadLoadAssetFailException : public RadException {
 public:
  template <typename... Args>
  RadLoadAssetFailException(const char* fmt, const Args&... args) : RadException(fmt, args...) {}
};

/**
 * @brief 用于构建渲染器实例
 */
class RAD_EXPORT_API BuildContext {
 public:
  //建造渲染器需要的资源
  void SetAssetManager(AssetManager& mgr);
  void SetFactoryManager(FactoryManager& mgr);
  const AssetManager& GetAssetManager() const;
  const FactoryManager& GetFactoryManager() const;

  //设置场景数据
  void SetCamera(ConfigNode cfg) { _cameraNode = cfg; }
  void SetSampler(ConfigNode cfg) { _samplerNode = cfg; }
  void SetGlobalMedium(ConfigNode cfg) { _globalMediumNode = cfg; }
  void SetBsdfVariables(ConfigNode cfg) { _bsdfVarsNode = cfg; }
  void SetScene(ConfigNode cfg) { _sceneNode = cfg; }
  void SetAssets(ConfigNode cfg) { _assetNode = cfg; }
  void SetAccel(ConfigNode cfg) { _accelNode = cfg; }
  void SetRenderer(ConfigNode cfg) { _rendererNode = cfg; }
  void SetDefaultAssetManager(const std::filesystem::path& workDir);
  void SetDefaultFactoryManager();
  void SetFromJson(nlohmann::json& json);

  //建造
  Unique<Renderer> Build();

 private:
  AssetManager* _assetMngr{nullptr};
  FactoryManager* _factoryMngr{nullptr};

  ConfigNode _cameraNode{};
  ConfigNode _samplerNode{};
  ConfigNode _globalMediumNode{};
  ConfigNode _bsdfVarsNode{};
  ConfigNode _sceneNode{};
  ConfigNode _assetNode{};
  ConfigNode _accelNode{};
  ConfigNode _rendererNode{};
  Unique<AssetManager> _defaultAssetMngr;
  Unique<FactoryManager> _defaultFactoryMngr;
};

}  // namespace Rad
