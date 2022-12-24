#pragma once

#include "types.h"
#include "color.h"

#include <vector>
#include <istream>

namespace Rad {

enum class ImageChannelType {
  UInt,
  Half,
  Float
};

/**
 * @brief 图片读取结果
 */
struct ImageReadResult {
  /**
   * @brief 字面意思，是否读取成功
   */
  bool IsSuccess;
  /**
   * @brief x 和 y 分量表示图片长宽，z 分量表示一个像素有多少通道
   */
  Eigen::Vector3i Size;
  /**
   * @brief 图片数据，数据格式：(y * width + x) * z * ChannelSize
   */
  std::vector<Byte> Data;
  /**
   * @brief 像素通道的类型
   */
  ImageChannelType ChannelType;
  /**
   * @brief 如果读取失败，会储存失败原因
   */
  std::string FailReason;
};

class RAD_EXPORT_API ImageReader {
 public:
  /**
   * @brief 使用stb_image读取图片，支持jpg、png、bmp、psd、tga、gif、pnm、pic
   *
   * @param stream 输入流
   * @param requireChannel 需要的通道数量，只能是1、3或4
   * @param isFlipY 是否需要在读取时翻转y轴
   */
  static ImageReadResult ReadLdrStb(
      std::istream& stream,
      Int32 requireChannel,
      bool isFlipY = true);

  static ImageReadResult ReadHdrStb(
      std::istream& stream,
      Int32 requireChannel,
      bool isFlipY = true);

  /**
   * @brief 读取exr格式图片，只支持读取RGB三通道
   *
   * @param stream 输入流
   */
  static ImageReadResult ReadExr(
      std::istream& stream);

  /**
   * @brief 写入exr格式图片，只支持写入RGB三通道
   *
   * @param stream 写入流
   * @param img 图片数据
   */
  static void WriteExr(std::ostream& stream, const MatrixX<Color24f>& img);
};

}  // namespace Rad
