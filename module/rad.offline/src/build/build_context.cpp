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
  //创建相机
  Unique<Camera> mainCamera;
  {
    std::string cameraType = GetTypeFromConfig(_cameraNode);
    CameraFactory* cameraFactory = _factoryMngr->GetFactory<CameraFactory>(cameraType);
    mainCamera = cameraFactory->Create(this, _cameraNode);
  }
  //创建采样器
  Unique<Sampler> samplerInstance;
  {
    std::string samplerType = GetTypeFromConfig(_samplerNode);
    SamplerFactory* samplerFactory = _factoryMngr->GetFactory<SamplerFactory>(samplerType);
    samplerInstance = samplerFactory->Create(this, _samplerNode);
  }
  //将采样器交给相机管理
  mainCamera->AttachSampler(std::move(samplerInstance));
  //创建全局参与介质
  Unique<Medium> globalMedium;
  {
    if (_globalMediumNode.GetData() != nullptr) {
      std::string mediumType = GetTypeFromConfig(_globalMediumNode);
      MediumFactory* mediumFactory = _factoryMngr->GetFactory<MediumFactory>(mediumType);
      globalMedium = mediumFactory->Create(this, Matrix4::Identity(), _globalMediumNode);
    }
  }
  //创建bsdf变量
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
  //创建形状
  std::vector<Unique<Bsdf>> bsdfs;
  std::vector<Unique<Shape>> shapes;
  std::vector<Unique<Light>> lights;
  std::vector<Unique<Medium>> mediums;
  {
    //形状存在子形状，因此需要BFS来遍历
    //遍历过程中计算Model矩阵，将形状变换到世界空间
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
      //尝试读取子形状，并放入队列
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
      //尝试读取形状
      Unique<Shape> shapeInstance;
      {
        ConfigNode shapeNode;
        if (entityNode.TryRead("shape", shapeNode)) {
          std::string type = GetTypeFromConfig(shapeNode);
          ShapeFactory* factory = _factoryMngr->GetFactory<ShapeFactory>(type);
          shapeInstance = factory->Create(this, ecfg.ToWorld, shapeNode);
        }
      }
      //尝试读取光源
      Unique<Light> lightInstance;
      {
        ConfigNode lightNode;
        if (entityNode.TryRead("light", lightNode)) {
          std::string type = GetTypeFromConfig(lightNode);
          LightFactory* factory = _factoryMngr->GetFactory<LightFactory>(type);
          lightInstance = factory->Create(this, ecfg.ToWorld, lightNode);
        }
      }
      //尝试读取BSDF
      //可能引用创建的BSDF遍历
      Unique<Bsdf> bsdfInstance;
      Bsdf* bsdfRef = nullptr;
      {
        ConfigNode bsdfNode;
        if (entityNode.TryRead("bsdf", bsdfNode)) {
          std::string bsdfRefName;
          //如果有ref字段，就去BSDF变量表里搜索
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
      //尝试创建参与介质
      //如果没有定义外部介质，则默认使用全局的介质。如果不想使用则需声明is_global字段
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
      //开始组装实体
      //如果形状附加了光源，但是没定义BSDF，就加上漫反射材质以免报错
      if (lightInstance != nullptr && shapeInstance != nullptr && bsdfInstance == nullptr) {
        BsdfFactory* factory = _factoryMngr->GetFactory<BsdfFactory>("diffuse");
        bsdfInstance = factory->Create(this, {});
      }
      //如果存在形状，就要将实体的BSDF、光源借用给形状
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
      //虽然只有面光源需要形状，但是我们还是让所有光源都借用形状
      if (lightInstance != nullptr) {
        lightInstance->AttachShape(shapeInstance.get());
      }
      Medium* inMedPtr = nullptr;
      Medium* outMedPtr = nullptr;
      //内部介质只能显示的定义出来
      if (mediumInsideInstance != nullptr) {
        inMedPtr = mediumInsideInstance.get();
      }
      //外部介质可能使用全局介质，需要特殊判断
      if (mediumOutsideInstance == nullptr) {
        if (isMediumOutsideUseGlobal && globalMedium != nullptr) {
          outMedPtr = globalMedium.get();
        }
      } else {
        outMedPtr = mediumOutsideInstance.get();
      }
      //将介质借用给形状
      if (shapeInstance != nullptr) {
        shapeInstance->AttachMedium(inMedPtr, outMedPtr);
      }
      //将所有实例移动给储存区域
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
  //别忘了将全局介质也移动到储存区域
  if (globalMedium != nullptr) {
    mediums.emplace_back(std::move(globalMedium));
  }
  shapes.shrink_to_fit();
  //创建加速结构
  Unique<Accel> accelInstance;
  {
    std::string accelType = GetTypeFromConfig(_accelNode);
    AccelFactory* factory = _factoryMngr->GetFactory<AccelFactory>(accelType);
    accelInstance = factory->Create(this, std::move(shapes), _accelNode);
  }
  //别忘了将BSDF移动到储存区域
  for (auto& [name, bsdfIns] : bsdfVariables) {
    bsdfs.emplace_back(std::move(bsdfIns));
  }
  //创建场景对象
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
