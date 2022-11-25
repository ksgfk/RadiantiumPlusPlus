#pragma once

#include "../common.h"
#include "../build/config_node.h"
#include "../utility/location_resolver.h"
#include "triangle_model.h"
#include "block_based_image.h"
#include "static_grid.h"

#include <string>

namespace Rad {

enum class AssetType {
  Model,
  Image,
  Volume
};

struct AssetLoadResult {
  bool IsSuccess;
  std::string FailResult;
};

/**
 * @brief 需要从其他地方, 比如硬盘、网络读取的数据都可以是资产
 */
class RAD_EXPORT_API Asset {
 public:
  inline Asset(BuildContext* ctx, const ConfigNode& cfg, AssetType type) : _type(type) {
    _name = cfg.Read<std::string>("name");
    _location = cfg.Read<std::string>("location");
  }
  virtual ~Asset() noexcept = default;

  AssetType GetType() const { return _type; }
  const std::string& Name() const { return _name; }
  const std::string& Name() { return _name; }
  const std::string& Location() const { return _location; }
  const std::string& Location() { return _location; }

  virtual AssetLoadResult Load(const LocationResolver& resolver) = 0;

 protected:
  const AssetType _type;
  std::string _name;
  std::string _location;
};

/**
 * @brief 模型资产的抽象, 一个模型里可能包含多个子模型, 也可以直接将所有子模型合并成完整模型获取
 */
class RAD_EXPORT_API ModelAsset : public Asset {
 public:
  ModelAsset(BuildContext* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Model) {}
  virtual ~ModelAsset() noexcept = default;

  virtual Share<TriangleModel> FullModel() const = 0;
  virtual Share<TriangleModel> GetSubModel(const std::string& name) const = 0;
  virtual bool HasSubModel(const std::string& name) const = 0;
};

/**
 * @brief 图片资产的抽象, 可以将读取数据转化为RGB或单通道图片
 *
 */
class RAD_EXPORT_API ImageAsset : public Asset {
 public:
  ImageAsset(BuildContext* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Image) {}
  virtual ~ImageAsset() noexcept = default;

  virtual Share<BlockBasedImage<Color>> ToImageRgb() const = 0;
  virtual Share<BlockBasedImage<Float32>> ToImageGray() const = 0;
};

class VolumeAsset : public Asset {
 public:
  VolumeAsset(BuildContext* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Volume) {}
  virtual ~VolumeAsset() noexcept = default;

  virtual Share<StaticGrid> GetGrid() const = 0;
  virtual Share<StaticGrid> GetGrid(const std::string& name) const = 0;
};

}  // namespace Rad
