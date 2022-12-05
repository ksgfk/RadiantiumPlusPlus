#pragma once

#include <embree3/rtcore.h>

#include <rad/core/types.h>

namespace Rad {

struct RgbSpectrum;

#if defined(RAD_USE_FLOAT32)
using Float = float;
#elif defined(RAD_USE_FLOAT64)
using Float = double;
#else
using Float = float;
#endif

using Spectrum = RgbSpectrum;

using Vector2 = Eigen::Vector2<Float>;
using Vector3 = Eigen::Vector3<Float>;
using Vector4 = Eigen::Vector4<Float>;
using Matrix3 = Eigen::Matrix3<Float>;
using Matrix4 = Eigen::Matrix4<Float>;
using BoundingBox2 = Eigen::AlignedBox<Float, 2>;
using BoundingBox3 = Eigen::AlignedBox<Float, 3>;

}  // namespace Rad

std::ostream& operator<<(std::ostream& os, enum RTCError err);
template <>
struct spdlog::fmt_lib::formatter<enum RTCError> : spdlog::fmt_lib::ostream_formatter {};
