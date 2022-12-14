#include <rad/offline/build/build_context.h>

#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/camera.h>
#include <rad/offline/render/sampler.h>
#include <rad/offline/render/medium.h>
#include <rad/offline/render/bsdf.h>
#include <rad/offline/render/light.h>
#include <rad/offline/render/shape.h>
#include <rad/offline/render/renderer.h>
#include <rad/offline/render/scene.h>

#include <queue>

namespace Rad {

void BuildContext::SetAssetManager(AssetManager& mgr) { _assetMngr = &mgr; }
void BuildContext::SetFactoryManager(FactoryManager& mgr) { _factoryMngr = &mgr; }
const AssetManager& BuildContext::GetAssetManager() const { return *_assetMngr; }
const FactoryManager& BuildContext::GetFactoryManager() const { return *_factoryMngr; }

void BuildContext::SetDefaultAssetManager(const std::filesystem::path& workDir) {
  _defaultAssetMngr = std::make_unique<AssetManager>();
  _defaultAssetMngr->SetObjectFactory(*_factoryMngr);
  _defaultAssetMngr->SetWorkDirectory(workDir);
  _defaultAssetMngr->SetImageStorageIsBlockBased(true);
  if (_assetNode.GetData() != nullptr) {
    for (auto&& i : _assetNode.As<std::vector<ConfigNode>>()) {
      _defaultAssetMngr->Load(i);
    }
  }
  SetAssetManager(*_defaultAssetMngr);
}

void BuildContext::SetDefaultFactoryManager() {
  _defaultFactoryMngr = std::make_unique<FactoryManager>();
  for (auto&& i : GetRadCoreAssetFactories()) {
    _defaultFactoryMngr->RegisterFactory(i());
  }
  for (auto&& i : GetRadOfflineFactories()) {
    _defaultFactoryMngr->RegisterFactory(i());
  }
  SetFactoryManager(*_defaultFactoryMngr);
}

void BuildContext::SetFromJson(nlohmann::json& json) {
  ConfigNode node(&json);
  SetCamera(node.Read<ConfigNode>("camera"));
  ConfigNode samplerNode;
  if (!node.TryRead("sampler", samplerNode)) {
    json["sampler"] = {{"type", "independent"}};
    samplerNode = ConfigNode(&json["sampler"]);
  }
  SetSampler(samplerNode);
  ConfigNode globalMediumNode;
  if (node.TryRead("global_medium", globalMediumNode)) {
    SetGlobalMedium(globalMediumNode);
  }
  ConfigNode bsdfVarNode;
  if (node.TryRead("bsdf_vars", bsdfVarNode)) {
    SetBsdfVariables(bsdfVarNode);
  }
  SetScene(node.Read<ConfigNode>("scene"));
  ConfigNode assetNode;
  if (node.TryRead("assets", assetNode)) {
    SetAssets(assetNode);
  }
  ConfigNode accelNode;
  if (!node.TryRead("accel", accelNode)) {
    json["accel"] = {{"type", "embree"}};
    accelNode = ConfigNode(&json["accel"]);
  }
  SetAccel(accelNode);
  SetRenderer(node.Read<ConfigNode>("renderer"));
}

static std::string GetTypeFromConfig(ConfigNode cfg) {
  std::string type = cfg.Read<std::string>("type");
  return type;
}

struct EntityConfig {
  ConfigNode Config;
  Matrix4 ToWorld = Matrix4::Identity();
};

Unique<Renderer> BuildContext::Build() {
  //????????????
  Unique<Camera> mainCamera;
  {
    std::string cameraType = GetTypeFromConfig(_cameraNode);
    CameraFactory* cameraFactory = _factoryMngr->GetFactory<CameraFactory>(cameraType);
    mainCamera = cameraFactory->Create(this, _cameraNode);
  }
  //???????????????
  Unique<Sampler> samplerInstance;
  {
    std::string samplerType = GetTypeFromConfig(_samplerNode);
    SamplerFactory* samplerFactory = _factoryMngr->GetFactory<SamplerFactory>(samplerType);
    samplerInstance = samplerFactory->Create(this, _samplerNode);
  }
  //??????????????????????????????
  mainCamera->AttachSampler(std::move(samplerInstance));
  //????????????????????????
  Unique<Medium> globalMedium;
  {
    if (_globalMediumNode.GetData() != nullptr) {
      std::string mediumType = GetTypeFromConfig(_globalMediumNode);
      MediumFactory* mediumFactory = _factoryMngr->GetFactory<MediumFactory>(mediumType);
      globalMedium = mediumFactory->Create(this, Matrix4::Identity(), _globalMediumNode);
    }
  }
  //??????bsdf??????
  std::map<std::string, Unique<Bsdf>> bsdfVariables;
  if (_bsdfVarsNode.GetData() != nullptr) {
    for (auto&& bsdfVar : _bsdfVarsNode.As<std::vector<ConfigNode>>()) {
      std::string name = bsdfVar.Read<std::string>("name");
      ConfigNode bsdfDefNode = bsdfVar.Read<ConfigNode>("bsdf");
      std::string bsdfType = GetTypeFromConfig(bsdfDefNode);
      BsdfFactory* bsdfFactory = _factoryMngr->GetFactory<BsdfFactory>(bsdfType);
      Unique<Bsdf> bsdfInstance = bsdfFactory->Create(this, bsdfDefNode);
      auto [iter, isInsert] = bsdfVariables.emplace(name, std::move(bsdfInstance));
      if (!isInsert) {
        throw RadArgumentException("duplicate bsdf var name: {}", name);
      }
    }
  }
  //????????????
  std::vector<Unique<Bsdf>> bsdfs;
  std::vector<Unique<Shape>> shapes;
  std::vector<Unique<Light>> lights;
  std::vector<Unique<Medium>> mediums;
  {
    //????????????????????????????????????BFS?????????
    //?????????????????????Model???????????????????????????????????????
    std::queue<EntityConfig> q;
    for (auto&& entityNode : _sceneNode.As<std::vector<ConfigNode>>()) {
      EntityConfig ecfg;
      ecfg.Config = entityNode;
      ConfigNode toWorldNode;
      if (entityNode.TryRead("to_world", toWorldNode)) {
        ecfg.ToWorld = ConfigNodeAsTransform(toWorldNode);
      }
      q.emplace(ecfg);
    }
    while (!q.empty()) {
      EntityConfig ecfg = q.front();
      q.pop();
      ConfigNode entityNode = ecfg.Config;
      //???????????????????????????????????????
      {
        ConfigNode childrenNodes;
        if (entityNode.TryRead("children", childrenNodes)) {
          for (const ConfigNode& childNode : childrenNodes.As<std::vector<ConfigNode>>()) {
            EntityConfig chcfg;
            chcfg.Config = childNode;
            ConfigNode toWorldNode;
            if (childNode.TryRead("to_world", toWorldNode)) {
              chcfg.ToWorld = ConfigNodeAsTransform(toWorldNode) * ecfg.ToWorld;
            } else {
              chcfg.ToWorld = ecfg.ToWorld;
            }
            q.emplace(chcfg);
          }
        }
      }
      //??????????????????
      Unique<Shape> shapeInstance;
      {
        ConfigNode shapeNode;
        if (entityNode.TryRead("shape", shapeNode)) {
          std::string type = GetTypeFromConfig(shapeNode);
          ShapeFactory* factory = _factoryMngr->GetFactory<ShapeFactory>(type);
          shapeInstance = factory->Create(this, ecfg.ToWorld, shapeNode);
        }
      }
      //??????????????????
      Unique<Light> lightInstance;
      {
        ConfigNode lightNode;
        if (entityNode.TryRead("light", lightNode)) {
          std::string type = GetTypeFromConfig(lightNode);
          LightFactory* factory = _factoryMngr->GetFactory<LightFactory>(type);
          lightInstance = factory->Create(this, ecfg.ToWorld, lightNode);
        }
      }
      //????????????BSDF
      //?????????????????????BSDF??????
      Unique<Bsdf> bsdfInstance;
      Bsdf* bsdfRef = nullptr;
      {
        ConfigNode bsdfNode;
        if (entityNode.TryRead("bsdf", bsdfNode)) {
          std::string bsdfRefName;
          //?????????ref???????????????BSDF??????????????????
          if (bsdfNode.TryRead("ref", bsdfRefName)) {
            auto findBsdfRefIter = bsdfVariables.find(bsdfRefName);
            if (findBsdfRefIter == bsdfVariables.end()) {
              throw RadArgumentException("unknown bsdf var: {}", bsdfRefName);
            }
            bsdfRef = findBsdfRefIter->second.get();
          } else {
            std::string type = GetTypeFromConfig(bsdfNode);
            BsdfFactory* factory = _factoryMngr->GetFactory<BsdfFactory>(type);
            bsdfInstance = factory->Create(this, bsdfNode);
          }
        }
      }
      //????????????????????????
      //????????????????????????????????????????????????????????????????????????????????????????????????is_global??????
      Unique<Medium> mediumInsideInstance;
      Unique<Medium> mediumOutsideInstance;
      bool isMediumOutsideUseGlobal = true;
      {
        ConfigNode inMediumNode;
        if (entityNode.TryRead("in_medium", inMediumNode)) {
          std::string type = GetTypeFromConfig(inMediumNode);
          MediumFactory* factory = _factoryMngr->GetFactory<MediumFactory>(type);
          mediumInsideInstance = factory->Create(this, ecfg.ToWorld, inMediumNode);
        }
        ConfigNode outMediumNode;
        if (entityNode.TryRead("out_medium", outMediumNode)) {
          if (!outMediumNode.TryRead("is_global", isMediumOutsideUseGlobal)) {
            std::string type = GetTypeFromConfig(outMediumNode);
            MediumFactory* factory = _factoryMngr->GetFactory<MediumFactory>(type);
            mediumOutsideInstance = factory->Create(this, ecfg.ToWorld, outMediumNode);
          }
        }
      }
      //??????????????????
      //?????????????????????????????????????????????BSDF???????????????????????????????????????
      if (lightInstance != nullptr && shapeInstance != nullptr && bsdfInstance == nullptr) {
        BsdfFactory* factory = _factoryMngr->GetFactory<BsdfFactory>("diffuse");
        bsdfInstance = factory->Create(this, {});
      }
      //???????????????????????????????????????BSDF????????????????????????
      if (shapeInstance != nullptr) {
        if (lightInstance != nullptr) {
          shapeInstance->AttachLight(lightInstance.get());
        }
        if (bsdfInstance != nullptr) {
          shapeInstance->AttachBsdf(bsdfInstance.get());
        }
        if (!shapeInstance->HasBsdf() && bsdfRef != nullptr) {
          shapeInstance->AttachBsdf(bsdfRef);
        }
      }
      //????????????????????????????????????????????????????????????????????????????????????
      if (lightInstance != nullptr) {
        lightInstance->AttachShape(shapeInstance.get());
      }
      Medium* inMedPtr = nullptr;
      Medium* outMedPtr = nullptr;
      //???????????????????????????????????????
      if (mediumInsideInstance != nullptr) {
        inMedPtr = mediumInsideInstance.get();
      }
      //?????????????????????????????????????????????????????????
      if (mediumOutsideInstance == nullptr) {
        if (isMediumOutsideUseGlobal && globalMedium != nullptr) {
          outMedPtr = globalMedium.get();
        }
      } else {
        outMedPtr = mediumOutsideInstance.get();
      }
      //????????????????????????
      if (shapeInstance != nullptr) {
        shapeInstance->AttachMedium(inMedPtr, outMedPtr);
      }
      //????????????????????????????????????
      if (bsdfInstance != nullptr) {
        bsdfs.emplace_back(std::move(bsdfInstance));
      }
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
  Medium* globalMediumPtr = globalMedium.get();
  //????????????????????????????????????????????????
  if (globalMedium != nullptr) {
    mediums.emplace_back(std::move(globalMedium));
  }
  shapes.shrink_to_fit();
  //??????????????????
  Unique<Accel> accelInstance;
  {
    std::string accelType = GetTypeFromConfig(_accelNode);
    AccelFactory* factory = _factoryMngr->GetFactory<AccelFactory>(accelType);
    accelInstance = factory->Create(this, std::move(shapes), _accelNode);
  }
  //????????????BSDF?????????????????????
  for (auto& [name, bsdfIns] : bsdfVariables) {
    bsdfs.emplace_back(std::move(bsdfIns));
  }
  //??????????????????
  bsdfs.shrink_to_fit();
  lights.shrink_to_fit();
  mediums.shrink_to_fit();
  Unique<Scene> sceneInstance = std::make_unique<Scene>(
      std::move(accelInstance),
      std::move(mainCamera),
      std::move(bsdfs),
      std::move(lights),
      std::move(mediums),
      globalMediumPtr);
  std::string type = GetTypeFromConfig(_rendererNode);
  RendererFactory* factory = _factoryMngr->GetFactory<RendererFactory>(type);
  Unique<Renderer> renderInstance = factory->Create(this, std::move(sceneInstance), _rendererNode);
  return std::move(renderInstance);
}

}  // namespace Rad
