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

/**
 * @brief 资源的抽象
 *
 */
class IAsset : public Object {
 public:
  virtual ~IAsset() noexcept = default;

  /*属性*/
  virtual bool IsValid() const = 0;
  virtual const std::string& Name() const = 0;
  virtual AssetType GetType() const = 0;

  /*方法*/
  virtual void Load(const LocationResolver& resolver) = 0;
};

/**
 * @brief 对于模型资源的抽象
 * 一个模型可能包含多个子模型, 通过名字来查询子模型
 * 模型是可以共享的, 所以返回shared指针
 */
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
std::unique_ptr<IFactory> CreateObjModelFactory();  //.obj格式的模型
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::AssetType type);
template <>
struct spdlog::fmt_lib::formatter<rad::AssetType> : spdlog::fmt_lib::ostream_formatter {};
