#pragma once

#include <Eigen/Dense>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(_DEBUG)
#define RAD_DEBUG_MODE
#endif

namespace rad {

#if defined(RAD_USE_FLOAT32)
using Float = float;
#elif defined(RAD_USE_FLOAT64)
using Float = double;
#else
using Float = float;
#endif

using Vec2 = Eigen::Vector2<Float>;
using Vec3 = Eigen::Vector3<Float>;
using Vec4 = Eigen::Vector4<Float>;
using Mat3 = Eigen::Matrix3<Float>;
using Mat4 = Eigen::Matrix4<Float>;

}  // namespace rad
