#pragma once

#include <string>
#include "static_buffer.h"
#include "spectrum.h"

namespace rad::image {

void SaveOpenExr(const std::string& file, const StaticBuffer2D<RgbSpectrum>& img);

}  // namespace rad
