#include <rad/core/asset.h>

#include <rad/core/volume_reader.h>

namespace Rad {

/**
 * @brief 读取.vdb格式文件
 */
class VolumeVdb final : public VolumeAsset {
 public:
  VolumeVdb(const AssetManager* ctx, const ConfigNode& cfg) : VolumeAsset(ctx, cfg) {}
  ~VolumeVdb() noexcept override = default;

  Share<VolumeGrid> GetGrid() const override {
    if (_grids.size() != 1) {
      throw RadArgumentException("vdb {} has more than one grid", _name);
    }
    return _grids[0].second;
  }

  Share<VolumeGrid> FindGrid(const std::string& name) const override {
    for (const auto& grid : _grids) {
      if (grid.first == name) {
        return grid.second;
      }
    }
    throw RadArgumentException("cannot find grid: {}", name);
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    auto result = VolumeReader::ReadVdb(*stream);
    if (!result.IsSuccess) {
      return AssetLoadResult{false, std::move(result.FailReason)};
    }
    _grids = std::move(result.Data);
    return AssetLoadResult{true};
  }

 private:
  std::vector<VolumeReadResult::Entry> _grids;
};

class VolumeVdbFactory final : public AssetFactory {
 public:
  VolumeVdbFactory() : AssetFactory("volume_data_vdb") {}
  ~VolumeVdbFactory() noexcept override = default;
  Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<VolumeVdb>(ctx, cfg);
  }
};

Unique<AssetFactory> _FactoryCreateVolumeVdbFunc_() {
  return std::make_unique<VolumeVdbFactory>();
}

}  // namespace Rad
