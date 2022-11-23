#include <rad/offline/asset/asset.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

namespace Rad {

class VolumeMitsubaVol final : public VolumeAsset {
 public:
  VolumeMitsubaVol(BuildContext* ctx, const ConfigNode& cfg) : VolumeAsset(ctx, cfg) {
    _logger = Logger::GetCategory("VolumeOpenVDB");
  }
  ~VolumeMitsubaVol() noexcept override = default;

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    try {
      char header[3];
      stream->read(header, 3);
      if (header[0] != 'V' || header[1] != 'O' || header[2] != 'L') {
        return AssetLoadResult{false, "不是有效的mitsuba3 vol文件"};
      }
      char version;
      stream->read(&version, 1);
      if (version != 3) {
        return AssetLoadResult{false, "只支持版本3的mitsuba3 vol文件"};
      }
      Int32 dataType;
      stream->read((char*)&dataType, 4);
      if (dataType != 1) {
        return AssetLoadResult{false, "mitsuba3 vol只支持Float32格式"};
      }
      Int32 sizeX, sizeY, sizeZ;
      stream->read((char*)&sizeX, 4);
      stream->read((char*)&sizeY, 4);
      stream->read((char*)&sizeZ, 4);
      Int32 channelCount;
      stream->read((char*)&channelCount, 4);
      if (channelCount != 1 && channelCount != 3) {
        return AssetLoadResult{false, "mitsuba3 vol只支持1或3通道"};
      }
      Float32 dims[6];
      stream->read((char*)dims, 24);
      Eigen::Vector3i size(sizeX, sizeY, sizeZ);
      BoundingBox3 bbox(
          Vector3(dims[0], dims[1], dims[2]),
          Vector3(dims[3], dims[4], dims[5]));
      size_t all = size.prod();
      std::vector<Float32> data;
      data.reserve(all * channelCount);
      for (size_t i = 0; i < all; i++) {
        if (channelCount == 1) {
          Float32 val;
          stream->read((char*)&val, 4);
          data.emplace_back(val);
        } else {
          Float32 val[3];
          stream->read((char*)val, 12);
          data.emplace_back(val[0]);
          data.emplace_back(val[1]);
          data.emplace_back(val[2]);
        }
      }
      _grid = std::make_shared<StaticGrid>(std::move(data), size, UInt32(channelCount), bbox);
      return AssetLoadResult{true};
    } catch (const std::exception& e) {
      return AssetLoadResult{false, e.what()};
    }
  }

  Share<StaticGrid> GetGrid() const override { return _grid; }
  Share<StaticGrid> GetGrid(const std::string& name) const override { return _grid; }

 private:
  Share<spdlog::logger> _logger;
  Share<StaticGrid> _grid;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(VolumeMitsubaVol, "volume_data_mitsuba_vol");
