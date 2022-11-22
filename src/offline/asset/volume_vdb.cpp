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
    AssetLoadResult result;
    try {
      openvdb::io::Stream vdbStream(*stream);
      auto grids = vdbStream.getGrids();
      for (const openvdb::GridBase::Ptr& grid : *grids) {
        _logger->debug("begin read grid {}", grid->getName());
        for (auto metaIt = grid->beginMeta(); metaIt != grid->endMeta(); metaIt++) {
          const auto& metadata = metaIt->second;
          _logger->debug("metadata name: {}, value: {} ", metaIt->first, metadata->str());
        }
        auto bboxDim = grid->evalActiveVoxelDim();
        auto bbox = grid->evalActiveVoxelBoundingBox();
        auto wsMin = grid->indexToWorld(bbox.min());
        auto wsMax = grid->indexToWorld(bbox.max() - openvdb::Vec3R(1, 1, 1));
        if (grid->isType<openvdb::FloatGrid>()) {
          Eigen::Vector3i size(bboxDim.x(), bboxDim.y(), bboxDim.z());
          Unique<Float32[]> data = std::make_unique<Float32[]>((size_t)size.prod());
          auto floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
          openvdb::tools::GridSampler<openvdb::FloatGrid, openvdb::tools::BoxSampler> sampler(*floatGrid);
          Float32* p = data.get();
          for (int k = bbox.min().z(); k < bbox.max().z(); ++k) {
            for (int j = bbox.min().y(); j < bbox.max().y(); ++j) {
              for (int i = bbox.min().x(); i < bbox.max().x(); ++i) {
                float value = sampler.isSample(openvdb::Vec3R(i, j, k));
                *p = value;
                p++;
              }
            }
          }
          BoundingBox3 bound(
              Vector3(Float(wsMin.x()), Float(wsMin.y()), Float(wsMin.z())),
              Vector3(Float(wsMax.x()), Float(wsMax.y()), Float(wsMax.z())));
          Share<StaticGrid> radGrid = std::make_shared<StaticGrid>(
              std::move(data),
              size,
              1,
              bound);
          _grids.emplace_back(GridEntry{std::move(radGrid), grid->getName()});
        } else if (grid->isType<openvdb::Vec3fGrid>()) {
          Eigen::Vector3i size(bboxDim.x(), bboxDim.y(), bboxDim.z());
          auto vec3fGrid = openvdb::gridPtrCast<openvdb::Vec3fGrid>(grid);
          openvdb::tools::GridSampler<openvdb::Vec3fGrid, openvdb::tools::BoxSampler> sampler(*vec3fGrid);
          std::vector<openvdb::Vec3f> data;
          data.reserve(size.prod());
          for (int k = bbox.min().z(); k < bbox.max().z(); ++k) {
            for (int j = bbox.min().y(); j < bbox.max().y(); ++j) {
              for (int i = bbox.min().x(); i < bbox.max().x(); ++i) {
                openvdb::Vec3f value = sampler.isSample(openvdb::Vec3R(i, j, k));
                data.emplace_back(value);
              }
            }
          }
          BoundingBox3 bound(
              Vector3(Float(wsMin.x()), Float(wsMin.y()), Float(wsMin.z())),
              Vector3(Float(wsMax.x()), Float(wsMax.y()), Float(wsMax.z())));
          Unique<Float32[]> realData = std::make_unique<Float32[]>(data.size() * 3);
          Float32* ptr = realData.get();
          for (const auto& d : data) {
            *ptr = d.x();
            ptr++;
            *ptr = d.y();
            ptr++;
            *ptr = d.z();
            ptr++;
          }
          Share<StaticGrid> radGrid = std::make_shared<StaticGrid>(
              std::move(realData),
              size,
              1,
              bound);
          _grids.emplace_back(GridEntry{std::move(radGrid), grid->getName()});
        } else {
          throw RadInvalidOperationException("目前只支持读取float和vec3f的grid");
        }
      }
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

  Share<spdlog::logger> _logger;
  std::vector<GridEntry> _grids;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(VolumeOpenVdb, "volume_vdb");
