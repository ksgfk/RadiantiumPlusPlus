#pragma once

#include "model.h"
#include <string>

namespace rad {

enum class AssetType {
  Model,
  Image
};

class IAsset {
 public:
  virtual ~IAsset() noexcept = default;

  virtual bool IsValid() const = 0;
  virtual AssetType GetType() const = 0;
};

class IModelAsset : public IAsset {
 public:
  virtual ~IModelAsset() noexcept = default;

  AssetType GetType() const final { return AssetType::Model; }
  virtual size_t SubModelCount() = 0;
  virtual std::shared_ptr<TriangleModel> FullModel() = 0;
  virtual std::shared_ptr<TriangleModel> GetSubModel(const std::string& name) = 0;
  virtual bool HasSubModel(const std::string& name) = 0;
};

}  // namespace rad
