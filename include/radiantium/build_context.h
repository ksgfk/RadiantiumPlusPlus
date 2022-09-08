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

/**
 * @brief 根据配置文件构造Renderer的主要类
 * 构造过程分为多个阶段
 */
class BuildContext {
 private:
  /*
   * 世界中的实体的配置
   */
  struct EntityConfig {
    Transform ToWorld;
    std::unique_ptr<IConfigNode> ShapeConfig;
  };

 public:
  /**
   * @brief 创建实体的上下文, 在构造实体组件时可以访问 (例如构造形状时, 可以访问ToWorld)
   */
  struct EntityCreateContext {
    Transform ToWorld;
  };

  BuildContext();

  /**
   * @brief 在 BuildStage::CollectFactory 阶段添加一个工厂类
   */
  void RegisterFactory(std::unique_ptr<IFactory> factory);
  /**
   * @brief 在 BuildStage::Init 阶段设置配置文件
   */
  void SetConfig(std::unique_ptr<IConfigNode> config);
  /**
   * @brief 在 BuildStage::Init 阶段设置工作目录, 用于加载资源时解析location
   */
  void SetWorkPath(const std::string& path);
  /**
   * @brief 在 BuildStage::Init 阶段设置最终输出的文件名
   */
  void SetOutputName(const std::string& name);
  /**
   * @brief 在 BuildStage::Init 阶段设置输出的文件夹, 如果不设置的话, 默认是工作目录
   */
  void SetOutputDir(const std::string& dir);
  /**
   * @brief 开始建造, 只能在 BuildStage::Init 阶段调用
   */
  void Build();
  /**
   * @brief 获取建造结果, 只能在 BuildStage::Done 阶段调用
   */
  std::pair<std::unique_ptr<IRenderer>, DataWriter> Result();
  /**
   * @brief 在任何阶段都可以获取工厂类, 如果获取失败会返回nullptr
   */
  IFactory* GetFactory(const std::string& name) const;
  /**
   * @brief 获取工厂类的同时进行类型转换, 转换失败会返回nullptr
   */
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
  /**
   * @brief 在任何阶段都可以获取模型资源, 如果获取失败会返回nullptr
   */
  IModelAsset* GetModel(const std::string& name) const;
  /**
   * @brief 只有在 BuildStage::CreateEntity 阶段才可以获取配置实体组件的上下文
   */
  const EntityCreateContext& GetEntityCreateContext() const;
  /**
   * @brief 只有在 BuildStage::BuildWorld 阶段才可以移动世界所有权
   */
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

  BuildStage _state = BuildStage::Init;     //记录当前阶段
  std::shared_ptr<spdlog::logger> _logger;  //专用的logger
  LocationResolver _resolver;
  DataWriter _writer;
  std::unique_ptr<IConfigNode> _config;  //配置文件根节点

  std::unordered_map<std::string, std::unique_ptr<IFactory>> _factories;  //存所有工厂, 用字符串存取

  std::vector<std::unique_ptr<IConfigNode>> _assetNodes;      //暂时存所有资源的配置节点
  std::unique_ptr<ICamera> _mainCamera;                       //主摄像机的实例
  std::unique_ptr<IConfigNode> _accelNode;                    //加速结构的配置节点
  std::unique_ptr<IConfigNode> _rendererNode;                 //渲染器的配置节点
  std::vector<std::unique_ptr<EntityConfig>> _entityConfigs;  //暂存所有实体的配置节点
  std::unique_ptr<ISampler> _samplerInstance;                 //采样器的配置节点

  std::unordered_map<std::string, std::shared_ptr<IAsset>> _assetInstances;  //资源的实例

  EntityCreateContext _entityCreateContext{};
  std::vector<Entity> _entityInstances;  //实体的实例
  size_t _hasShapeEntityCount = 0;       //记录拥有形状的实体的数量

  std::unique_ptr<ITracingAccel> _accelInstance;  //加速结构实例

  std::unique_ptr<World> _world;
  std::unique_ptr<IRenderer> _renderer;
};

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::BuildStage stage);
template <>
struct spdlog::fmt_lib::formatter<rad::BuildStage> : spdlog::fmt_lib::ostream_formatter {};
