#include <rad/core/factory.h>

namespace Rad {

void FactoryManager::RegisterFactory(Unique<Factory> factory) {
  std::string name = factory->GetName();
  auto [iter, isIn] = _factories.emplace(name, std::move(factory));
  if (!isIn) {
    throw RadArgumentException("duplicate factory {}", name);
  }
}

Factory* FactoryManager::GetFactory(const std::string& name) const {
  auto iter = _factories.find(name);
  if (iter == _factories.end()) {
    throw RadArgumentException("cannot find factory: {}", name);
  }
  return iter->second.get();
}

}  // namespace Rad
