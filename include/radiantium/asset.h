#pragma once

#include "model.h"
#include "object.h"
#include "location_resolver.h"
#include "fwd.h"
#include "static_buffer.h"

#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace rad {

class BlockBasedImage {
 public:
  constexpr static UInt32 BlockSize = 32;

  struct Block {
    std::vector<Float> Data;
  };

  BlockBasedImage(UInt32 width, UInt32 height, UInt32 depth);
  BlockBasedImage(UInt32 width, UInt32 height, UInt32 depth, const Float* data);

  UInt32 Width() const { return _width; }
  UInt32 Height() const { return _height; }
  UInt32 Depth() const { return _depth; }

  Float Read1(UInt32 x, UInt32 y) const;
  RgbSpectrum Read3(UInt32 x, UInt32 y) const;

 private:
  std::pair<UInt32, UInt32> GetIndex(UInt32 x, UInt32 y) const;

  const UInt32 _width;
  const UInt32 _height;
  const UInt32 _depth;
  UInt32 _widthBlockCount;
  std::vector<Block> _data;
};

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

/**
 * @brief 对图片资源的抽象
 *
 */
class IImageAsset : public IAsset {
 public:
  virtual ~IImageAsset() noexcept = default;

  AssetType GetType() const final { return AssetType::Image; }

  virtual std::shared_ptr<BlockBasedImage> Image() const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateObjModelFactory();      //.obj格式的模型
std::unique_ptr<IFactory> CreateDefaultImageFactory();  // 默认的图片加载器
}  // namespace factory

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::AssetType type);
template <>
struct spdlog::fmt_lib::formatter<rad::AssetType> : spdlog::fmt_lib::ostream_formatter {};
