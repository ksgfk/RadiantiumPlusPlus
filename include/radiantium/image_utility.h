#pragma once

#include <string>
#include "static_buffer.h"
#include "spectrum.h"

namespace rad::image {
/**
 * @brief 将图片储存为.exr格式
 */
void SaveOpenExr(const std::string& file, const StaticBuffer2D<RgbSpectrum>& img);

}  // namespace rad::image
