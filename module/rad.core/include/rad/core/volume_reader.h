#pragma once

#include "types.h"
#include "volume_grid.h"

#include <utility>

namespace Rad {

struct VolumeReadResult {
  /**
   * @brief item1是名字，item2是数据
   *
   */
  using Entry = std::pair<std::string, Share<VolumeGrid>>;
  /**
   * @brief 字面意思，是否读取成功
   */
  bool IsSuccess;
  /**
   * @brief 数据
   */
  std::vector<Entry> Data;
  /**
   * @brief 如果读取失败，会储存失败原因
   */
  std::string FailReason;
};

/**
 * @brief 体积数据读取
 */
class RAD_EXPORT_API VolumeReader {
 public:
  /**
   * @brief 加载.vdb格式
   *
   * @param stream 输入流
   */
  static VolumeReadResult ReadVdb(std::istream& stream);
  /**
   * @brief 加载mitsuba3的.vol格式
   *
   * @param stream 输入流
   */
  static VolumeReadResult ReadMitsubaVol(std::istream& stream);
};

}  // namespace Rad
