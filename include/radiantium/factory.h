#pragma once

#include <memory>
#include <string>

#include "fwd.h"

namespace rad {
/**
 * @brief 工厂抽象
 */
class IFactory {
 public:
  virtual ~IFactory() noexcept = default;
  virtual std::string UniqueId() const = 0;
};

class ITracingAccelFactory : public IFactory {
 public:
  virtual ~ITracingAccelFactory() noexcept = default;
  virtual std::unique_ptr<ITracingAccel> Create(
      const BuildContext* context,
      const IConfigNode* config) const = 0;
};

class ICameraFactory : public IFactory {
 public:
  virtual ~ICameraFactory() noexcept = default;
  virtual std::unique_ptr<ICamera> Create(const BuildContext* context, const IConfigNode* config) const = 0;
};

class IShapeFactory : public IFactory {
 public:
  virtual ~IShapeFactory() noexcept = default;
  virtual std::unique_ptr<IShape> Create(const BuildContext* context, const IConfigNode* config) const = 0;
};

class IAssetFactory : public IFactory {
 public:
  virtual ~IAssetFactory() noexcept = default;
  virtual std::unique_ptr<IAsset> Create(const BuildContext* context, const IConfigNode* config) const = 0;
};

class ISamplerFactory : public IFactory {
 public:
  virtual ~ISamplerFactory() noexcept = default;
  virtual std::unique_ptr<ISampler> Create(const BuildContext* context, const IConfigNode* config) const = 0;
};

class IRendererFactory : public IFactory {
 public:
  virtual ~IRendererFactory() noexcept = default;
  virtual std::unique_ptr<IRenderer> Create(BuildContext* context, const IConfigNode* config) const = 0;
};

namespace factory_help {
/**
 * @brief 获取系统里所有工厂类 (就是硬编码
 */
void RegisterSystemFactories(BuildContext* context);
}  // namespace factory_help

}  // namespace rad
