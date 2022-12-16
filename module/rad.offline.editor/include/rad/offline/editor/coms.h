#pragma once

#include <rad/core/config_node.h>
#include <rad/core/asset.h>

#include <variant>

namespace Rad {

class OfflineConfig {
 public:
  virtual ~OfflineConfig() noexcept = default;

  virtual void OnGui() = 0;
};

class MeshModelConfig : public OfflineConfig {
 public:
  ~MeshModelConfig() noexcept override = default;

  void OnGui() override;

 private:
  Share<ModelAsset> _model;
  std::string _subModel;
};

class DiffuseBsdfConfig : public OfflineConfig {
 public:
  ~DiffuseBsdfConfig() noexcept override = default;

  void OnGui() override;

 private:
  Unique<OfflineConfig> _reflection;
};

}  // namespace Rad
