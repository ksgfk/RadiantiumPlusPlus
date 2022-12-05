#pragma once

#include "types.h"
#include "factory.h"
#include "location_resolver.h"
#include "triangle_model.h"
#include "volume_grid.h"
#include "memory.h"

namespace Rad {

class AssetManager;

enum class AssetType {
  Model,
  Image,
  Volume
};

struct AssetLoadResult {
  bool IsSuccess;
  std::string FailReason;
};

/**
 * @brief 需要从其他地方, 比如硬盘、网络读取的数据都可以是资产
 */
class RAD_EXPORT_API Asset {
 public:
  Asset(const AssetManager* ctx, const ConfigNode& cfg, AssetType type);
  virtual ~Asset() noexcept = default;

  AssetType GetType() const { return _type; }
  const std::string& GetName() const { return _name; }
  const std::string& GetLocation() const { return _location; }

  virtual AssetLoadResult Load(const LocationResolver& resolver) = 0;

 protected:
  AssetType _type;
  std::string _name;
  std::string _location;
};

/**
 * @brief 模型资产的抽象, 一个模型里可能包含多个子模型, 也可以直接将所有子模型合并成完整模型获取
 */
class RAD_EXPORT_API ModelAsset : public Asset {
 public:
  ModelAsset(const AssetManager* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Model) {}
  virtual ~ModelAsset() noexcept = default;

  virtual Share<TriangleModel> FullModel() const = 0;
  virtual Share<TriangleModel> GetSubModel(const std::string& name) const = 0;
  virtual bool HasSubModel(const std::string& name) const = 0;
};

/**
 * @brief 图片资产的抽象, 可以将读取数据转化为RGB或单色纹理
 */
class RAD_EXPORT_API ImageAsset : public Asset {
 public:
  ImageAsset(const AssetManager* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Image) {}
  virtual ~ImageAsset() noexcept = default;

  virtual Share<MatrixX<Color24f>> ToImageRgb() const = 0;
  virtual Share<MatrixX<Float32>> ToImageMono() const = 0;
  virtual Share<BlockArray2D<Color24f>> ToBlockImageRgb() const = 0;
  virtual Share<BlockArray2D<Float32>> ToBlockImageMono() const = 0;

  /**
   * @brief 因为离线渲染使用纹理时需要分块储存来加速纹理采样过程，因此我们留个函数来生成分块储存的纹理
   */
  virtual void GenerateBlockBasedImage() = 0;
};

/**
 * @brief 三维体积资产的抽象，可能包含多个体积数据
 */
class RAD_EXPORT_API VolumeAsset : public Asset {
 public:
  VolumeAsset(const AssetManager* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Volume) {}
  virtual ~VolumeAsset() noexcept = default;

  virtual Share<VolumeGrid> GetGrid() const = 0;
  virtual Share<VolumeGrid> FindGrid(const std::string& name) const = 0;
};

/**
 * @brief 使用引用计数管理资产，允许加载和卸载资源
 */
class RAD_EXPORT_API AssetManager {
 public:
  AssetManager();
  AssetManager(const AssetManager&) = delete;
  AssetManager& operator=(const AssetManager&) = delete;

  AssetLoadResult Load(ConfigNode cfg);
  void Unload(const std::string& name);
  /**
   * @brief 回收引用计数为1，也就是只有AssetManager引用的资产
   */
  void GarbageCollect();
  /**
   * @brief 应该最先设置工厂管理
   */
  void SetObjectFactory(const FactoryManager& manager);
  /**
   * @brief 如果不设置搜索路径，则加载资源时只搜索运行目录下的文件
   */
  void SetWorkDirectory(const std::filesystem::path& path);
  void SetImageStorageIsBlockBased(bool isBlock) { _isImageBlock = isBlock; }

  bool IsLoaded(const std::string& name) const;
  /**
   * @brief 引用资源，会使计数器+1
   */
  Share<Asset> Reference(const std::string& name) const;
  template <typename T>
  Share<T> Reference(const std::string& name) const {
    Share<Asset> asset = Reference(name);
#if defined(RAD_DEFINE_DEBUG)
    return std::dynamic_pointer_cast<T, Asset>(asset);
#else
    return std::static_pointer_cast<T, Asset>(asset);
#endif
  }
  /**
   * @brief 临时借用资源，不会使计数器+1，应该谨慎使用，避免出现悬空引用
   */
  const Asset* Borrow(const std::string& name) const;
  template <typename T>
  const T* Borrow(const std::string& name) const {
    const Asset* asset = Borrow(name);
#if defined(RAD_DEFINE_DEBUG)
    const T* typedAsset = dynamic_cast<const T*>(asset);
    return typedAsset;
#else
    const T* typedAsset = static_cast<const T*>(asset);
    return typedAsset;
#endif
  }
  const LocationResolver& GetLocationResolver() const { return *_resolver; }
  bool IsImageStorageBlockBased() const { return _isImageBlock; }

 private:
  const FactoryManager* _factory{nullptr};
  Unique<LocationResolver> _resolver;
  std::map<std::string, Share<Asset>> _assets;
  Share<spdlog::logger> _logger;
  bool _isImageBlock{false};
};

class RAD_EXPORT_API AssetFactory : public Factory {
 public:
  AssetFactory(const std::string& name) : Factory(name) {}
  virtual ~AssetFactory() noexcept override = default;
  virtual Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const = 0;
};

RAD_EXPORT_API std::vector<std::function<Unique<Factory>(void)>> GetRadCoreAssetFactories();

}  // namespace Rad
