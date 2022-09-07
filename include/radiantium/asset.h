#pragma once

#include "model.h"
#include "object.h"
#include "location_resolver.h"
#include "fwd.h"

#include <string>

namespace rad {

enum class AssetType {
  Model,
  Image
};

class IAsset : public Object {
 public:
  virtual ~IAsset() noexcept = default;

  virtual bool IsValid() const = 0;
  virtual const std::string& Name() const = 0;
  virtual AssetType GetType() const = 0;
  virtual void Load(const LocationResolver& resolver) = 0;
};

class IModelAsset : public IAsset {
 public:
  virtual ~IModelAsset() noexcept = default;

  AssetType GetType() const final { return AssetType::Model; }
  virtual size_t SubModelCount() const = 0;
  virtual std::shared_ptr<TriangleModel> FullModel() const = 0;
  virtual std::shared_ptr<TriangleModel> GetSubModel(const std::string& name) const = 0;
  virtual bool HasSubModel(const std::string& name) const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateObjModelFactory();
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::AssetType type);
template <>
struct spdlog::fmt_lib::formatter<rad::AssetType> : spdlog::fmt_lib::ostream_formatter {};
