#pragma once

#include "../common.h"

#include <ostream>

namespace Rad::Image {
/**
 * @brief 输出exr图片
 *
 * @param stream 必须是二进制流, 也就是创建时一定要std::ios::binary
 */
void SaveOpenExr(std::ostream& stream, const MatrixX<Color>& img);

}  // namespace Rad::Image
