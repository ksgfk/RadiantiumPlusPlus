#pragma once

#include <string>
#include "static_buffer.h"
#include "spectrum.h"

namespace rad {

void SaveOpenExr(const std::string& file, const StaticBuffer<RgbSpectrum>& img);

}  // namespace rad
