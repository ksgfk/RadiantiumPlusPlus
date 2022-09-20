#pragma once

#include "../common.h"
#include "../build/config_node.h"
#include "../utility/location_resolver.h"
#include "triangle_model.h"
#include "block_based_image.h"

#include <string>

namespace Rad {

enum class AssetType {
  Model,
  Image
};

struct AssetLoadResult {
  bool IsSuccess;
  std::string FailResult;
};

class Asset {
 public:
  inline Asset(BuildContext* ctx, const ConfigNode& cfg, AssetType type) : _type(type) {
    _name = cfg.Read<std::string>("name");
  }
  virtual ~Asset() noexcept = default;

  AssetType GetType() const { return _type; }
  const std::string& Name() const { return _name; }
  const std::string& Name() { return _name; }

  virtual AssetLoadResult Load(const LocationResolver& resolver) = 0;

 protected:
  const AssetType _type;
  std::string _name;
};

class ModelAsset : public Asset {
 public:
  ModelAsset(BuildContext* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Model) {}
  virtual ~ModelAsset() noexcept = default;

  virtual Share<TriangleModel> FullModel() const = 0;
  virtual Share<TriangleModel> GetSubModel(const std::string& name) const = 0;
  virtual bool HasSubModel(const std::string& name) const = 0;
};

class ImageAsset : public Asset {
 public:
  ImageAsset(BuildContext* ctx, const ConfigNode& cfg) : Asset(ctx, cfg, AssetType::Image) {}
  virtual ~ImageAsset() noexcept = default;

  virtual Share<BlockBasedImage<Color>> ToImageRgb() const = 0;
  virtual Share<BlockBasedImage<Float32>> ToImageGray() const = 0;
};

}  // namespace Rad
