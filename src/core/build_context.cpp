#include <radiantium/build_context.h>

#include <radiantium/factory.h>
#include <radiantium/config_node.h>

#include <ostream>

namespace rad {

BuildContext::BuildContext() {
  _logger = logger::GetCategoryLogger("BuildContext");
}

void BuildContext::RegisterFactory(std::unique_ptr<IFactory>&& factory) {
  if (_state != BuildStage::CollectFactory) {
    _logger->error("{} can only call at {}", "BuildContext::RegisterFactory", BuildStage::CollectFactory);
    return;
  }
  if (_factories.find(factory->UniqueId()) != _factories.end()) {
    _logger->error("exist factory {}", factory->UniqueId());
    return;
  }
  _factories.emplace(factory->UniqueId(), std::move(factory));
}

void BuildContext::SetConfig(std::unique_ptr<IConfigNode>&& config) {
  if (_state != BuildStage::Init) {
    _logger->error("{} can only call at {}", "BuildContext::SetConfig", BuildStage::Init);
    return;
  }
  _config = std::move(config);
}

void BuildContext::Build() {
  if (_state != BuildStage::Init) {
    _logger->error("{} can only call at {}", "BuildContext::Build", BuildStage::Init);
    return;
  }
  _state = BuildStage::CollectFactory;
  CollectFactory();
}

void BuildContext::CollectFactory() {
  factory_help::RegisterSystemFactories(this);
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::BuildStage stage) {
  switch (stage) {
    case rad::BuildStage::Init:
      os << "Init";
      break;
    case rad::BuildStage::CollectFactory:
      os << "CollectFactory";
      break;
    case rad::BuildStage::ParseConfig:
      os << "ParseConfig";
      break;
    case rad::BuildStage::LoadAsset:
      os << "LoadAsset";
      break;
    case rad::BuildStage::CreateEntity:
      os << "CreateEntity";
      break;
    case rad::BuildStage::BuildAccel:
      os << "BuildAccel";
      break;
    case rad::BuildStage::BuildScene:
      os << "BuildScene";
      break;
    case rad::BuildStage::Done:
      os << "Done";
      break;
  }
  return os;
}
