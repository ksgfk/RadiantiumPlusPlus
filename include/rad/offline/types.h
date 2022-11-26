#pragma once

#include <embree3/rtcore.h>

#include <rad/core/types.h>

#if defined(__APPLE__)
#define RAD_PLATFORM_MACOS
#elif defined(__linux__)
#define RAD_PLATFORM_LINUX
#elif defined(_WIN32) || defined(_WIN64)
#define RAD_PLATFORM_WINDOWS
#endif

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

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

// 照着抄的 https://fmt.dev/latest/api.html#format-api
#if defined(RAD_USE_FLOAT64)
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector2> {
  template <typename FormatContext>
  auto format(const Rad::Vector2& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}>", v[0], v[1]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector3> {
  template <typename FormatContext>
  auto format(const Rad::Vector3& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector4> {
  template <typename FormatContext>
  auto format(const Rad::Vector4& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}, {}>", v[0], v[1], v[2], v[3]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Matrix3> : spdlog::fmt_lib::ostream_formatter {};
template <>
struct spdlog::fmt_lib::formatter<Rad::Matrix4> : spdlog::fmt_lib::ostream_formatter {};
#endif
std::ostream& operator<<(std::ostream& os, enum RTCError err);
template <>
struct spdlog::fmt_lib::formatter<enum RTCError> : spdlog::fmt_lib::ostream_formatter {};
