#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "radiantium.h"

namespace rad {

class IFactory;
class IConfigNode;

enum class BuildStage {
  Init,
  CollectFactory,
  ParseConfig,
  LoadAsset,
  CreateEntity,
  BuildAccel,
  BuildScene,
  Done
};

class BuildContext {
 public:
  BuildContext();

  void RegisterFactory(std::unique_ptr<IFactory>&& factory);
  void SetConfig(std::unique_ptr<IConfigNode>&& config);
  void Build();

 private:
  void CollectFactory();

  BuildStage _state = BuildStage::Init;
  std::shared_ptr<spdlog::logger> _logger;
  std::unordered_map<std::string, std::unique_ptr<IFactory>> _factories;
  std::unique_ptr<IConfigNode> _config;
};

}  // namespace rad

std::ostream& operator<<(std::ostream& os, rad::BuildStage stage);
template <>
struct spdlog::fmt_lib::formatter<rad::BuildStage> : spdlog::fmt_lib::ostream_formatter {};
