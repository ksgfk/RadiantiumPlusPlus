#include <rad/core/volume_reader.h>

#include <openvdb/openvdb.h>
#include <openvdb/io/Stream.h>
#include <openvdb/tools/Interpolation.h>

namespace Rad {

template <class VdbGridType, int ChannelCount>
static VolumeReadResult::Entry GetGridEntry(const openvdb::GridBase::Ptr& grid) {
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
  BoundingBox3f bound(
      Vector3f(Float32(wsMin.x()), Float32(wsMin.y()), Float32(wsMin.z())),
      Vector3f(Float32(wsMax.x()), Float32(wsMax.y()), Float32(wsMax.z())));
  Share<VolumeGrid> radGrid = std::make_shared<VolumeGrid>(std::move(data), size, ChannelCount, bound);
  return std::make_pair(grid->getName(), std::move(radGrid));
}

VolumeReadResult VolumeReader::ReadVdb(std::istream& stream) {
  VolumeReadResult result{};
  try {
    openvdb::io::Stream vdbStream(stream);
    auto grids = vdbStream.getGrids();
    std::vector<VolumeReadResult::Entry> data;
    for (const openvdb::GridBase::Ptr& grid : *grids) {
      if (grid->isType<openvdb::FloatGrid>()) {
        data.emplace_back(GetGridEntry<openvdb::FloatGrid, 1>(grid));
      } else if (grid->isType<openvdb::Vec3fGrid>()) {
        data.emplace_back(GetGridEntry<openvdb::Vec3fGrid, 3>(grid));
      } else {
        throw RadInvalidOperationException("目前只支持读取float和vec3f的grid");
      }
    }
    result.IsSuccess = true;
    result.Data = std::move(data);
  } catch (const std::exception& e) {
    result.IsSuccess = false;
    result.FailReason = e.what();
  }
  return result;
}

VolumeReadResult VolumeReader::ReadMitsubaVol(std::istream& stream) {
  VolumeReadResult result{};
  try {
    char header[3];
    stream.read(header, 3);
    if (header[0] != 'V' || header[1] != 'O' || header[2] != 'L') {
      throw RadArgumentException("不是有效的mitsuba3 vol文件");
    }
    char version;
    stream.read(&version, 1);
    if (version != 3) {
      throw RadArgumentException("只支持版本3的mitsuba3 vol文件");
    }
    Int32 dataType;
    stream.read((char*)&dataType, 4);
    if (dataType != 1) {
      throw RadArgumentException("mitsuba3 vol只支持Float32格式");
    }
    Int32 sizeX, sizeY, sizeZ;
    stream.read((char*)&sizeX, 4);
    stream.read((char*)&sizeY, 4);
    stream.read((char*)&sizeZ, 4);
    Int32 channelCount;
    stream.read((char*)&channelCount, 4);
    if (channelCount != 1 && channelCount != 3) {
      throw RadArgumentException("mitsuba3 vol只支持1或3通道");
    }
    Float32 dims[6];
    stream.read((char*)dims, 24);
    Eigen::Vector3i size(sizeX, sizeY, sizeZ);
    BoundingBox3f bbox(
        Vector3f(dims[0], dims[1], dims[2]),
        Vector3f(dims[3], dims[4], dims[5]));
    size_t all = size.prod();
    std::vector<Float32> data;
    data.reserve(all * channelCount);
    for (size_t i = 0; i < all; i++) {
      if (channelCount == 1) {
        Float32 val;
        stream.read((char*)&val, 4);
        data.emplace_back(val);
      } else {
        Float32 val[3];
        stream.read((char*)val, 12);
        data.emplace_back(val[0]);
        data.emplace_back(val[1]);
        data.emplace_back(val[2]);
      }
    }
    result.IsSuccess = true;
    result.Data.reserve(1);
    result.Data.emplace_back(std::make_pair(
        std::string(),
        std::make_shared<VolumeGrid>(std::move(data), size, UInt32(channelCount), bbox)));
  } catch (const std::exception& e) {
    result.IsSuccess = false;
    result.FailReason = e.what();
  }
  return result;
}

}  // namespace Rad
