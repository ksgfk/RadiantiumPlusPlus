#include <rad/offline/build/build_context.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/build/factory_func.h>

#include <unordered_map>
#include <queue>

namespace Rad {

class BuildContextImpl {
 public:
  BuildContextImpl(
      BuildContext* ctxPtr,
      const std::filesystem::path& workDir,
      const std::string& projName,
      ConfigNode cfg)
      : _ctxPtr(ctxPtr), _root(cfg) {
    _logger = Rad::Logger::GetCategory("build");
    _resolver = std::make_unique<LocationResolver>(workDir, projName);
  }

  void RegisterFactory(Unique<Factory> factory) {
    auto [iter, isIn] = _factory.try_emplace(factory->Name(), std::move(factory));
    if (!isIn) {
      throw RadArgumentException("已存在的工厂类: {}", factory->Name());
    }
    _logger->debug("register factory: {}", iter->first);
  }

  Factory* GetFactory(const std::string& name) const {
    auto iter = _factory.find(name);
    if (iter == _factory.end()) {
      throw RadArgumentException("不存在的工厂类: {}", name);
    }
    return iter->second.get();
  }

  template <typename T>
  T* GetFactory(const std::string& name) const {
    Factory* ptr = GetFactory(name);
    if (ptr == nullptr) {
      return nullptr;
    }
#ifdef RAD_DEBUG_MODE
    T* cast = dynamic_cast<T*>(ptr);
    if (cast == nullptr) {
      return nullptr;
    }
#else
    T* cast = static_cast<T*>(ptr);
#endif
    return cast;
  }

  ImageAsset* GetImage(const std::string& name) const {
    Asset* asset = FindAsset(name);
    if (asset->GetType() != AssetType::Image) {
      throw RadArgumentException("资源不是 Image: {}", name);
    }
    return static_cast<ImageAsset*>(asset);
  }

  ModelAsset* GetModel(const std::string& name) const {
    Asset* asset = FindAsset(name);
    if (asset->GetType() != AssetType::Model) {
      throw RadArgumentException("资源不是 Model: {}", name);
    }
    return static_cast<ModelAsset*>(asset);
  }

  const Matrix4& GetShapeMatrix() const {
    return _shapeToWorld;
  }

  std::vector<Unique<Shape>>&& GetShapes() {
    return std::move(_shapes);
  }

  Unique<Scene> GetScene() {
    return std::move(_scene);
  }

  std::pair<Unique<Renderer>, Unique<LocationResolver>> Build() {
    for (const auto& func : Rad::Factories::GetFactoryCreateFuncs()) {
      auto factory = func();
      RegisterFactory(std::move(factory));
    }
    LoadAsset();
    _scene = std::make_unique<Scene>(BuildScene());
    ConfigNode renderNode = _root.Read<ConfigNode>("renderer");
    std::string type = GetTypeFromConfig(renderNode);
    RendererFactory* factory = GetFactory<RendererFactory>(type);
    Unique<Renderer> renderInstance = factory->Create(_ctxPtr, renderNode);
    return std::make_pair(
        std::move(renderInstance),
        std::move(_resolver));
  }

 private:
  Asset* FindAsset(const std::string& name) const {
    auto iter = _asset.find(name);
    if (iter == _asset.end()) {
      throw RadArgumentException("不存在的资源: {}", name);
    }
    return iter->second.get();
  }

  void LoadAsset() {
    std::vector<ConfigNode> assetNodes;
    if (!_root.TryRead<std::vector<ConfigNode>>("assets", assetNodes)) {
      return;
    }
    for (const ConfigNode& assetNode : assetNodes) {
      std::string type = GetTypeFromConfig(assetNode);
      AssetFactory* factory = GetFactory<AssetFactory>(type);
      Unique<Asset> assetInstance = factory->Create(_ctxPtr, assetNode);
      Stopwatch sw;
      sw.Start();
      AssetLoadResult result = assetInstance->Load(*_resolver);
      sw.Stop();
      if (result.IsSuccess) {
        auto [iter, isIn] = _asset.emplace(assetInstance->Name(), std::move(assetInstance));
        if (!isIn) {
          throw RadLoadAssetFailException("已存在的资源 {}", iter->second->Name());
        }
        _logger->info("load asset {} , {} ms", iter->second->Name(), sw.ElapsedMilliseconds());
      } else {
        throw RadLoadAssetFailException("无法加载资源 {}, 原因: {}", assetInstance->Name(), result.FailResult);
      }
    }
  }

  struct EntityConfig {
    ConfigNode Config;
    Matrix4 ToWorld = Matrix4::Identity();
  };

  Scene BuildScene() {
    std::vector<ConfigNode> worldNodes;
    if (!_root.TryRead<std::vector<ConfigNode>>("scene", worldNodes)) {
      throw RadArgumentException("配置文件必须有 scene 属性");
    }
    Unique<Camera> mainCamera;
    Unique<Medium> globalMedium;
    {
      ConfigNode cameraNode;
      std::string cameraType;
      if (_root.TryRead<ConfigNode>("camera", cameraNode)) {
        cameraType = GetTypeFromConfig(cameraNode);
      } else {
        cameraType = "perspective";
      }
      CameraFactory* cameraFactory = GetFactory<CameraFactory>(cameraType);
      mainCamera = cameraFactory->Create(_ctxPtr, cameraNode);

      Unique<Sampler> samplerInstance;
      {
        ConfigNode samplerNode;
        std::string samplerType;
        if (_root.TryRead<ConfigNode>("sampler", samplerNode)) {
          samplerType = GetTypeFromConfig(samplerNode);
        } else {
          samplerType = "independent";
        }
        SamplerFactory* samplerFactory = GetFactory<SamplerFactory>(samplerType);
        samplerInstance = samplerFactory->Create(_ctxPtr, samplerNode);
      }
      mainCamera->AttachSampler(std::move(samplerInstance));

      {
        ConfigNode globalMediumNode;
        if (_root.TryRead<ConfigNode>("global_medium", globalMediumNode)) {
          std::string globalMediumType = GetTypeFromConfig(globalMediumNode);
          MediumFactory* mediumFactory = GetFactory<MediumFactory>(globalMediumType);
          globalMedium = mediumFactory->Create(_ctxPtr, globalMediumNode);
        }
      }
    }
    std::vector<Unique<Shape>> shapes;
    std::vector<Unique<Light>> lights;
    std::vector<Unique<Medium>> mediums;
    {
      std::queue<EntityConfig> q;
      for (const ConfigNode& entityNode : worldNodes) {
        EntityConfig ecfg;
        ecfg.Config = entityNode;
        ConfigNode toWorldNode;
        if (entityNode.TryRead("to_world", toWorldNode)) {
          ecfg.ToWorld = toWorldNode.AsTransform();
        }
        q.emplace(ecfg);
      }
      while (!q.empty()) {
        EntityConfig ecfg = q.front();
        q.pop();
        ConfigNode entityNode = ecfg.Config;
        {
          ConfigNode childrenNodes;
          if (entityNode.TryRead("children", childrenNodes)) {
            for (const ConfigNode& childNode : childrenNodes.As<std::vector<ConfigNode>>()) {
              EntityConfig chcfg;
              chcfg.Config = childNode;
              ConfigNode toWorldNode;
              if (childNode.TryRead("to_world", toWorldNode)) {
                chcfg.ToWorld = toWorldNode.AsTransform();
              }
              q.emplace(chcfg);
            }
          }
        }
        _shapeToWorld = ecfg.ToWorld;
        Unique<Shape> shapeInstance;
        {
          ConfigNode shapeNode;
          if (entityNode.TryRead("shape", shapeNode)) {
            std::string type = GetTypeFromConfig(shapeNode);
            ShapeFactory* factory = GetFactory<ShapeFactory>(type);
            shapeInstance = factory->Create(_ctxPtr, shapeNode);
          }
        }
        Unique<Light> lightInstance;
        {
          ConfigNode lightNode;
          if (entityNode.TryRead("light", lightNode)) {
            std::string type = GetTypeFromConfig(lightNode);
            LightFactory* factory = GetFactory<LightFactory>(type);
            lightInstance = factory->Create(_ctxPtr, lightNode);
          }
        }
        Unique<Bsdf> bsdfInstance;
        {
          ConfigNode bsdfNode;
          if (entityNode.TryRead("bsdf", bsdfNode)) {
            std::string type = GetTypeFromConfig(bsdfNode);
            BsdfFactory* factory = GetFactory<BsdfFactory>(type);
            bsdfInstance = factory->Create(_ctxPtr, bsdfNode);
          }
        }
        Unique<Medium> mediumInsideInstance;
        Unique<Medium> mediumOutsideInstance;
        bool isMediumOutsideUseGlobal;
        {
          ConfigNode inMediumNode;
          if (entityNode.TryRead("in_medium", inMediumNode)) {
            std::string type = GetTypeFromConfig(inMediumNode);
            MediumFactory* factory = GetFactory<MediumFactory>(type);
            mediumInsideInstance = factory->Create(_ctxPtr, inMediumNode);
          }
          ConfigNode outMediumNode;
          if (entityNode.TryRead("out_medium", outMediumNode)) {
            if (!outMediumNode.TryRead("is_global", isMediumOutsideUseGlobal)) {
              std::string type = GetTypeFromConfig(outMediumNode);
              MediumFactory* factory = GetFactory<MediumFactory>(type);
              mediumOutsideInstance = factory->Create(_ctxPtr, outMediumNode);
            }
          }
        }

        if (lightInstance != nullptr &&
            shapeInstance != nullptr &&
            bsdfInstance == nullptr) {
          BsdfFactory* factory = GetFactory<BsdfFactory>("diffuse");
          bsdfInstance = factory->Create(_ctxPtr, {});
        }

        if (shapeInstance != nullptr) {
          if (lightInstance != nullptr) {
            shapeInstance->AttachLight(lightInstance.get());
          }
          if (bsdfInstance != nullptr) {
            shapeInstance->AttachBsdf(std::move(bsdfInstance));
          }
        }
        if (lightInstance != nullptr) {
          lightInstance->AttachShape(shapeInstance.get());
        }
        Medium* inMedPtr = nullptr;
        Medium* outMedPtr = nullptr;
        if (mediumInsideInstance != nullptr) {
          inMedPtr = mediumInsideInstance.get();
        }
        if (mediumOutsideInstance == nullptr) {
          if (isMediumOutsideUseGlobal && globalMedium != nullptr) {
            outMedPtr = globalMedium.get();
          }
        } else {
          outMedPtr = mediumOutsideInstance.get();
        }
        shapeInstance->AttachMedium(inMedPtr, outMedPtr);

        if (shapeInstance != nullptr) {
          shapes.emplace_back(std::move(shapeInstance));
        }
        if (lightInstance != nullptr) {
          lights.emplace_back(std::move(lightInstance));
        }
        if (mediumInsideInstance != nullptr) {
          mediums.emplace_back(std::move(mediumInsideInstance));
        }
        if (mediumOutsideInstance != nullptr) {
          mediums.emplace_back(std::move(mediumOutsideInstance));
        }
      }
    }

    _shapes = std::move(shapes);
    if (globalMedium != nullptr) {
      mediums.emplace_back(std::move(globalMedium));
    }

    Unique<Accel> accelInstance;
    {
      ConfigNode accelNode;
      std::string type;
      if (_root.TryRead<ConfigNode>("accel", accelNode)) {
        type = GetTypeFromConfig(accelNode);
      } else {
        type = "embree";
      }
      AccelFactory* factory = GetFactory<AccelFactory>(type);
      accelInstance = factory->Create(_ctxPtr, accelNode);
    }
    return Scene(
        std::move(accelInstance),
        std::move(mainCamera),
        std::move(lights),
        std::move(mediums));
  }

  static std::string GetTypeFromConfig(const ConfigNode& cfg) {
    std::string type = cfg.Read<std::string>("type");
    return type;
  }

  BuildContext* _ctxPtr;
  Share<spdlog::logger> _logger;
  ConfigNode _root;
  Unique<LocationResolver> _resolver;
  std::unordered_map<std::string, Unique<Factory>> _factory;
  std::unordered_map<std::string, Unique<Asset>> _asset;
  Matrix4 _shapeToWorld;
  std::vector<Unique<Shape>> _shapes;
  Unique<Scene> _scene;
};

BuildContext::BuildContext(
    const std::filesystem::path& workDir,
    const std::string& projName,
    ConfigNode* cfg)
    : _impl(new BuildContextImpl(this, workDir, projName, *cfg)) {}

BuildContext::~BuildContext() noexcept {
  if (_impl != nullptr) {
    delete _impl;
    _impl = nullptr;
  }
}

void BuildContext::RegisterFactory(Unique<Factory> factory) {
  _impl->RegisterFactory(std::move(factory));
}

Factory* BuildContext::GetFactory(const std::string& name) const {
  return _impl->GetFactory(name);
}

ImageAsset* BuildContext::GetImage(const std::string& name) const {
  return _impl->GetImage(name);
}

ModelAsset* BuildContext::GetModel(const std::string& name) const {
  return _impl->GetModel(name);
}

const Matrix4& BuildContext::GetShapeMatrix() const {
  return _impl->GetShapeMatrix();
}

std::vector<Unique<Shape>>&& BuildContext::GetShapes() {
  return std::move(_impl->GetShapes());
}

Unique<Scene> BuildContext::GetScene() {
  return _impl->GetScene();
}

std::pair<Unique<Renderer>, Unique<LocationResolver>> BuildContext::Build() {
  return _impl->Build();
}

}  // namespace Rad
