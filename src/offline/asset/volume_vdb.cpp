#include <rad/offline/asset/asset.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>

#include <openvdb/openvdb.h>
#include <openvdb/io/Stream.h>
#include <openvdb/tools/Interpolation.h>

#include <optional>

namespace Rad {

class VolumeOpenVdb final : public VolumeAsset {
 public:
  VolumeOpenVdb(BuildContext* ctx, const ConfigNode& cfg) : VolumeAsset(ctx, cfg) {
    _logger = Logger::GetCategory("VolumeOpenVDB");
  }
  ~VolumeOpenVdb() noexcept override = default;

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, std::ios::binary);
    AssetLoadResult result{};
    try {
      openvdb::io::Stream vdbStream(*stream);
      auto grids = vdbStream.getGrids();
      for (const openvdb::GridBase::Ptr& grid : *grids) {
        _logger->debug("begin read grid {}", grid->getName());
        for (auto metaIt = grid->beginMeta(); metaIt != grid->endMeta(); metaIt++) {
          const auto& metadata = metaIt->second;
          _logger->debug("metadata name: {}, value: {} ", metaIt->first, metadata->str());
        }
        if (grid->isType<openvdb::FloatGrid>()) {
          _grids.emplace_back(GetGridEntry<openvdb::FloatGrid, 1>(grid));
        } else if (grid->isType<openvdb::Vec3fGrid>()) {
          _grids.emplace_back(GetGridEntry<openvdb::Vec3fGrid, 3>(grid));
        } else {
          throw RadInvalidOperationException("目前只支持读取float和vec3f的grid");
        }
      }
      result.IsSuccess = true;
    } catch (const std::exception& e) {
      result.IsSuccess = false;
      result.FailResult = e.what();
    }
    return result;
  }

  Share<StaticGrid> GetGrid() const override {
    if (_grids.size() != 1) {
      throw RadArgumentException("资源 {} 不只有一个静态网格", _name);
    }
    return _grids[0].Data;
  }

  Share<StaticGrid> GetGrid(const std::string& name) const override {
    for (const auto& grid : _grids) {
      if (grid.Name == name) {
        return grid.Data;
      }
    }
    throw RadArgumentException("找不到静态网格: {}", name);
  }

 private:
  struct GridEntry {
    Share<StaticGrid> Data;
    std::string Name;
  };

  template <class VdbGridType, int ChannelCount>
  static GridEntry GetGridEntry(const openvdb::GridBase::Ptr& grid) {
    auto bboxDim = grid->evalActiveVoxelDim();
    auto bbox = grid->evalActiveVoxelBoundingBox();
    auto wsMin = grid->indexToWorld(bbox.min());
    auto wsMax = grid->indexToWorld(bbox.max() - openvdb::Vec3R(1, 1, 1));
    Eigen::Vector3i size(bboxDim.x() - 1, bboxDim.y() - 1, bboxDim.z() - 1);
    auto gridType = openvdb::gridPtrCast<VdbGridType>(grid);
    openvdb::tools::GridSampler<VdbGridType, openvdb::tools::BoxSampler> sampler(*gridType);
    std::vector<Float32> data;
    data.reserve(size.prod() * ChannelCount);
    for (int k = bbox.min().z(); k < bbox.max().z(); k++) {
      for (int j = bbox.min().y(); j < bbox.max().y(); j++) {
        for (int i = bbox.min().x(); i < bbox.max().x(); i++) {
          auto value = sampler.isSample(openvdb::Vec3R(i, j, k));
          if constexpr (ChannelCount == 1) {
            data.emplace_back(value);
          } else {
            for (int l = 0; l < ChannelCount; l++) {
              data.emplace_back(value(l));
            }
          }
        }
      }
    }
    BoundingBox3 bound(
        Vector3(Float(wsMin.x()), Float(wsMin.y()), Float(wsMin.z())),
        Vector3(Float(wsMax.x()), Float(wsMax.y()), Float(wsMax.z())));
    Share<StaticGrid> radGrid = std::make_shared<StaticGrid>(std::move(data), size, ChannelCount, bound);
    return GridEntry{std::move(radGrid), grid->getName()};
  }

  Share<spdlog::logger> _logger;
  std::vector<GridEntry> _grids;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(VolumeOpenVdb, "volume_vdb");
