#include <rad/core/asset.h>

#include <rad/core/volume_reader.h>

namespace Rad {

/**
 * @brief 读取mitsuba3的.vol格式文件
 */
class VolumeMitsubaVol final : public VolumeAsset {
 public:
  VolumeMitsubaVol(const AssetManager* ctx, const ConfigNode& cfg) : VolumeAsset(ctx, cfg) {}
  ~VolumeMitsubaVol() noexcept override = default;

  Share<VolumeGrid> GetGrid() const override { return _grid; }
  Share<VolumeGrid> FindGrid(const std::string& name) const override { return _grid; }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    auto result = VolumeReader::ReadMitsubaVol(*stream);
    if (!result.IsSuccess) {
      return AssetLoadResult{false, std::move(result.FailReason)};
    }
    _grid = std::move(result.Data[0].second);
    return AssetLoadResult{true};
  }

 private:
  Share<VolumeGrid> _grid;
};

class VolumeMitsubaVolFactory final : public AssetFactory {
 public:
  VolumeMitsubaVolFactory() : AssetFactory("volume_data_mitsuba_vol") {}
  ~VolumeMitsubaVolFactory() noexcept override = default;
  Unique<Asset> Create(const AssetManager* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<VolumeMitsubaVol>(ctx, cfg);
  }
};

Unique<AssetFactory> _FactoryCreateVolumeMitsubaVolFunc_() {
  return std::make_unique<VolumeMitsubaVolFactory>();
}

}  // namespace Rad
