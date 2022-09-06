#pragma once

#include <memory>
#include <string>

namespace rad {

class IConfigNode;
class Object;
class BuildContext;

class IFactory {
 public:
  virtual ~IFactory() noexcept = default;

  virtual std::string UniqueId() const = 0;
  virtual std::unique_ptr<Object> Create(const BuildContext* context, const IConfigNode* config) const = 0;
};

namespace factory_help {
void RegisterSystemFactories(BuildContext* context);
}

}  // namespace rad
