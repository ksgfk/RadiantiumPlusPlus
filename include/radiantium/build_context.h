#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "radiantium.h"
#include "transform.h"
#include "location_resolver.h"
#include "asset.h"
#include "world.h"
#include "camera.h"

namespace rad {

class IFactory;
class IConfigNode;
class Object;

enum class BuildStage {
  Init,
  CollectFactory,
  ParseConfig,
  LoadAsset,
  CreateEntity,
  BuildAccel,
  BuildWorld,
  Done
};

class BuildContext {
 private:
  struct EntityConfig {
    Transform ToWorld;
    std::unique_ptr<IConfigNode> ShapeConfig;
  };

 public:
  struct EntityCreateContext {
    Transform ToWorld;
  };

  BuildContext();

  void RegisterFactory(std::unique_ptr<IFactory> factory);
  void SetConfig(std::unique_ptr<IConfigNode> config);
  void SetLocationResolver(const LocationResolver&);
  void SetOutputName(const std::string& name);
  void Build();
  std::pair<std::unique_ptr<IRenderer>, DataWriter> Result();

  IFactory* GetFactory(const std::string& name) const;
  template <typename T>
  T* GetFactory(const std::string& name) const {
    IFactory* ptr = GetFactory(name);
    if (ptr == nullptr) {
      return nullptr;
    }
#ifdef RAD_DEBUG_MODE
    T* cast = dynamic_cast<T*>(ptr);
    if (cast == nullptr) {
      _logger->error("can't cast factory type, factory name {}", name);
    }
#else
    T* cast = static_cast<T*>(ptr);
#endif
    return cast;
  }
  IModelAsset* GetModel(const std::string& name) const;
  const EntityCreateContext& GetEntityCreateContext() const { return _entityCreateContext; }
  std::unique_ptr<World> MoveWorld();

 private:
  void CollectFactory();
  void ParseConfig();
  void LoadAsset();
  void CreateEntity();
  void BuildAccel();
  void BuildWorld();

  IFactory* GetFactoryFromConfig(IConfigNode* config);
  template <typename T>
  T* GetFactoryFromConfig(IConfigNode* config) {
    IFactory* ptr = GetFactoryFromConfig(config);
    if (ptr == nullptr) {
      return nullptr;
    }
#ifdef RAD_DEBUG_MODE
    T* cast = dynamic_cast<T*>(ptr);
    if (cast == nullptr) {
      _logger->error("can't cast factory type");
    }
#else
    T* cast = static_cast<T*>(ptr);
#endif
    return cast;
  }

  BuildStage _state = BuildStage::Init;
  std::shared_ptr<spdlog::logger> _logger;
  LocationResolver _resolver;
  DataWriter _writer;
  std::unique_ptr<IConfigNode> _config;

  std::unordered_map<std::string, std::unique_ptr<IFactory>> _factories;

  std::vector<std::unique_ptr<IConfigNode>> _assetNodes;
  std::unique_ptr<ICamera> _mainCamera;
  std::unique_ptr<IConfigNode> _accelNode;
  std::unique_ptr<IConfigNode> _rendererNode;
  std::vector<std::unique_ptr<EntityConfig>> _entityConfigs;
  std::unique_ptr<ISampler> _samplerInstance;

  std::unordered_map<std::string, std::shared_ptr<IAsset>> _assetInstances;

  EntityCreateContext _entityCreateContext{};
  std::vector<Entity> _entityInstances;
  size_t _hasShapeEntityCount = 0;

  std::unique_ptr<ITracingAccel> _accelInstance;

  std::unique_ptr<World> _world;
  std::unique_ptr<IRenderer> _renderer;
};

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::BuildStage stage);
template <>
struct spdlog::fmt_lib::formatter<rad::BuildStage> : spdlog::fmt_lib::ostream_formatter {};
