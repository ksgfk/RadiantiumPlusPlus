#include <radiantium/build_context.h>

#include <radiantium/object.h>
#include <radiantium/factory.h>
#include <radiantium/config_node.h>
#include <radiantium/shape.h>
#include <radiantium/tracing_accel.h>
#include <radiantium/renderer.h>
#include <radiantium/asset.h>

#include <ostream>
#include <queue>
#include <utility>

namespace rad {

BuildContext::BuildContext() {
  _logger = logger::GetCategoryLogger("BuildContext");
}

void BuildContext::RegisterFactory(std::unique_ptr<IFactory> factory) {
  if (_state != BuildStage::CollectFactory) {
    _logger->error("{} can only call at {}", "BuildContext::RegisterFactory", BuildStage::CollectFactory);
    return;
  }
  if (_factories.find(factory->UniqueId()) != _factories.end()) {
    _logger->error("exist factory {}", factory->UniqueId());
    return;
  }
  _factories.emplace(factory->UniqueId(), std::move(factory));
}

void BuildContext::SetConfig(std::unique_ptr<IConfigNode> config) {
  if (_state != BuildStage::Init) {
    _logger->error("{} can only call at {}", "BuildContext::SetConfig", BuildStage::Init);
    return;
  }
  _config = std::move(config);
}

void BuildContext::SetLocationResolver(const LocationResolver& resolver) {
  _resolver = resolver;
  _writer.SavePath = _resolver.SearchPath;
}

void BuildContext::SetOutputName(const std::string& name) {
  _writer.FileName = name;
}

IFactory* BuildContext::GetFactory(const std::string& name) const {
  auto iter = _factories.find(name);
  if (iter == _factories.end()) {
    _logger->error("no factory named {}", name);
    return nullptr;
  }
  return iter->second.get();
}

IModelAsset* BuildContext::GetModel(const std::string& name) const {
  auto iter = _assetInstances.find(name);
  if (iter == _assetInstances.end()) {
    _logger->error("no model named: {}", name);
    return nullptr;
  }
  IAsset* ptr = iter->second.get();
#ifdef RAD_DEBUG_MODE
  IModelAsset* cast = dynamic_cast<IModelAsset*>(ptr);
  if (cast == nullptr) {
    _logger->error("can't cast asset type, asset name: {}, real type {}", name, ptr->GetType());
  }
#else
  IModelAsset* cast = static_cast<IModelAsset*>(ptr);
#endif
  return cast;
}

std::unique_ptr<World> BuildContext::MoveWorld() {
  return std::move(_world);
}

std::pair<std::unique_ptr<IRenderer>, DataWriter> BuildContext::Result() {
  return std::make_pair(std::move(_renderer), _writer);
}

void BuildContext::Build() {
  if (_state != BuildStage::Init) {
    _logger->error("{} can only call at {}", "BuildContext::Build", BuildStage::Init);
    return;
  }
  _state = BuildStage::CollectFactory;
  CollectFactory();
  _state = BuildStage::ParseConfig;
  _logger->info("parse config...");
  ParseConfig();
  _state = BuildStage::LoadAsset;
  LoadAsset();
  _state = BuildStage::CreateEntity;
  CreateEntity();
  _state = BuildStage::BuildAccel;
  _logger->info("build accel...");
  BuildAccel();
  _state = BuildStage::BuildWorld;
  BuildWorld();
  _state = BuildStage::Done;
  _logger->info("done.");
}

void BuildContext::CollectFactory() {
  //在这里硬编码了, 反射? 不会!
  factory_help::RegisterSystemFactories(this);
}

IFactory* BuildContext::GetFactoryFromConfig(IConfigNode* config) {
  auto type = config->GetString("type");
  auto factory = GetFactory(type);
  return factory;
}

struct EntityHierarchy {
  std::unique_ptr<IConfigNode> Config;
  EntityHierarchy* Parent;
  std::vector<std::unique_ptr<EntityHierarchy>> Children;
  Mat4 LocalTransform;
};

void BuildContext::ParseConfig() {
  /////////////////////////////////////////
  // Asset System (?
  if (_config->HasProperty("assets")) {
    //资源等会儿加载
    _assetNodes = _config->GetArray("assets");
  }
  /////////////////////////////////////////
  // Main Camera
  if (_config->HasProperty("camera")) {
    auto cameraNode = _config->GetObject("camera");
    auto nodePtr = cameraNode.get();
    _mainCamera = GetFactoryFromConfig<ICameraFactory>(nodePtr)->Create(this, nodePtr);
  } else {
    _mainCamera = GetFactory<ICameraFactory>("perspective")->Create(this, IConfigNode::Empty());
  }
  /////////////////////////////////////////
  // Integrator
  if (_config->HasProperty("renderer")) {
    _rendererNode = _config->GetObject("renderer");
  } else {
    _logger->error("config must have node {}", "integrator");
  }
  /////////////////////////////////////////
  // Tracing Accel
  if (_config->HasProperty("accel")) {
    _accelNode = _config->GetObject("accel");
  } else {
    _accelNode = nullptr;
  }
  /////////////////////////////////////////
  // Sampler
  if (_config->HasProperty("sampler")) {
    auto samplerNode = _config->GetObject("sampler");
    auto nodePtr = samplerNode.get();
    _samplerInstance = GetFactoryFromConfig<ISamplerFactory>(nodePtr)->Create(this, nodePtr);
  } else {
    _samplerInstance = GetFactory<ISamplerFactory>(
                           "independent")
                           ->Create(this, IConfigNode::Empty());
  }
  /////////////////////////////////////////
  // World
  if (_config->HasProperty("world")) {
    size_t entityCount = 0;
    std::queue<std::unique_ptr<EntityHierarchy>> rootQue;
    //构造实体的亲子关系
    {
      auto entityList = _config->GetArray("world");
      std::queue<EntityHierarchy> hieQue;  //将直接连接到根的实体存入队列
      for (auto&& entityNode : entityList) {
        hieQue.emplace(EntityHierarchy{std::move(entityNode), nullptr, {}, {}});
      }
      while (!hieQue.empty()) {  // BFS遍历所有实体
        auto& hie = hieQue.front();
        auto newEntity = std::make_unique<EntityHierarchy>(std::move(hie));
        hieQue.pop();
        if (newEntity->Config->HasProperty("children")) {  //将子节点存入队列, 等待遍历
          auto childrenList = newEntity->Config->GetArray("children");
          for (auto&& childNode : childrenList) {
            hieQue.emplace(EntityHierarchy{std::move(childNode), newEntity.get(), {}, {}});
          }
        }
        if (newEntity->Config->HasProperty("to_world")) {  //同时获取该节点变换到亲节点空间的变换矩阵
          Mat4 toWorld = IConfigNode::GetTransform(newEntity->Config.get(), "to_world");
          newEntity->LocalTransform = toWorld;
        } else {
          newEntity->LocalTransform = Mat4::Identity();
        }
        if (newEntity->Parent == nullptr) {
          rootQue.emplace(std::move(newEntity));
        } else {
          newEntity->Parent->Children.emplace_back(std::move(newEntity));
        }
        entityCount++;
      }
    }
    std::vector<std::unique_ptr<EntityConfig>> flatEntity;  //解除亲子关系, 变换矩阵直接变换到世界空间
    flatEntity.reserve(entityCount);
    while (!rootQue.empty()) {
      auto& hie = rootQue.front();
      auto newEntity = std::make_unique<EntityConfig>();
      auto trans = hie->LocalTransform;
      if (hie->Parent != nullptr) {
        trans = hie->Parent->LocalTransform * trans;
      }
      newEntity->ToWorld = Transform(trans);
      if (hie->Config->HasProperty("shape")) {
        newEntity->ShapeConfig = hie->Config->GetObject("shape");
        _hasShapeEntityCount++;
      }
      flatEntity.emplace_back(std::move(newEntity));
      rootQue.pop();
    }
    _entityConfigs = std::move(flatEntity);
  } else {
    _logger->error("config must have node {}", "world");
  }
}

void BuildContext::LoadAsset() {
  std::vector<std::shared_ptr<IAsset>> assets;
  assets.reserve(_assetNodes.size());
  for (const auto& assetConfig : _assetNodes) {
    auto assetFactory = GetFactoryFromConfig<IAssetFactory>(assetConfig.get());
    auto asset = assetFactory->Create(this, assetConfig.get());
    std::shared_ptr<Object> shared = std::move(asset);
    assets.emplace_back(std::static_pointer_cast<IAsset, Object>(std::move(shared)));
  }
  for (auto& asset : assets) {
    asset->Load(_resolver);
    if (asset->IsValid()) {
      _assetInstances.emplace(asset->Name(), asset);
      _logger->info("load asset {}", asset->Name());
    } else {
      _logger->error("invalid asset {}. ignore", asset->Name());
    }
  }
}

void BuildContext::CreateEntity() {
  _entityInstances.reserve(_entityConfigs.size());
  std::vector<Entity> hasShapeEntity;
  hasShapeEntity.reserve(_hasShapeEntityCount);
  std::vector<Entity> noShapeEntity;
  noShapeEntity.reserve(_entityConfigs.size() - _hasShapeEntityCount);
  for (auto& e : _entityConfigs) {
    Entity newEntity;
    _entityCreateContext.ToWorld = e->ToWorld;
    if (e->ShapeConfig != nullptr) {
      auto shapeFactory = GetFactoryFromConfig<IShapeFactory>(e->ShapeConfig.get());
      auto shapeInst = shapeFactory->Create(this, e->ShapeConfig.get());
      newEntity._shape = std::move(shapeInst);
    }
    if (newEntity.HasShape()) {
      hasShapeEntity.emplace_back(std::move(newEntity));
    } else {
      noShapeEntity.emplace_back(std::move(newEntity));
    }
  }
  for (auto& e : hasShapeEntity) {
    _entityInstances.emplace_back(std::move(e));
  }
  _hasShapeEntityCount = hasShapeEntity.size();
  for (auto& e : noShapeEntity) {
    _entityInstances.emplace_back(std::move(e));
  }
}

void BuildContext::BuildAccel() {
  if (_accelNode == nullptr) {
    _accelInstance = GetFactory<ITracingAccelFactory>("embree")->Create(
        this,
        IConfigNode::Empty());
  } else {
    _accelInstance = GetFactoryFromConfig<ITracingAccelFactory>(
                         _accelNode.get())
                         ->Create(this, _accelNode.get());
  }
  std::vector<IShape*> entityPtr(_hasShapeEntityCount);
  for (size_t i = 0; i < _hasShapeEntityCount; i++) {
    entityPtr[i] = _entityInstances[i].Shape();
  }
  _accelInstance->Build(std::move(entityPtr));
}

void BuildContext::BuildWorld() {
  _world = std::make_unique<World>();
  _world->_accel = std::move(_accelInstance);
  _world->_entity = std::move(_entityInstances);
  _world->_sampler = std::move(_samplerInstance);
  _world->_camera = std::move(_mainCamera);

  auto f = GetFactoryFromConfig<IRendererFactory>(_rendererNode.get());
  _renderer = f->Create(this, _rendererNode.get());
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::BuildStage stage) {
  switch (stage) {
    case rad::BuildStage::Init:
      os << "Init";
      break;
    case rad::BuildStage::CollectFactory:
      os << "CollectFactory";
      break;
    case rad::BuildStage::ParseConfig:
      os << "ParseConfig";
      break;
    case rad::BuildStage::LoadAsset:
      os << "LoadAsset";
      break;
    case rad::BuildStage::CreateEntity:
      os << "CreateEntity";
      break;
    case rad::BuildStage::BuildAccel:
      os << "BuildAccel";
      break;
    case rad::BuildStage::BuildWorld:
      os << "BuildWorld";
      break;
    case rad::BuildStage::Done:
      os << "Done";
      break;
  }
  return os;
}
