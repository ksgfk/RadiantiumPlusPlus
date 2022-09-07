#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <chrono>

#include "fwd.h"
#include "logger.h"
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <embree3/rtcore.h>

#include <Eigen/Dense>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(_DEBUG) || defined(RAD_DEFINE_DEBUG)
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

using Byte = std::byte;
using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;
using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using Float32 = std::float_t;
using Float64 = std::double_t;

using Vec2 = Eigen::Vector2<Float>;
using Vec3 = Eigen::Vector3<Float>;
using Vec4 = Eigen::Vector4<Float>;
using Mat3 = Eigen::Matrix3<Float>;
using Mat4 = Eigen::Matrix4<Float>;

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::duration<long long, std::chrono::milliseconds>;

}  // namespace rad

// see https://fmt.dev/latest/api.html#format-api
template <>
struct spdlog::fmt_lib::formatter<rad::Vec2> {
  template <typename FormatContext>
  auto format(const rad::Vec2& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}>", v[0], v[1]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<rad::Vec3> {
  template <typename FormatContext>
  auto format(const rad::Vec3& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<rad::Vec4> {
  template <typename FormatContext>
  auto format(const rad::Vec4& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}, {}>", v[0], v[1], v[2], v[3]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<rad::Mat3> : spdlog::fmt_lib::ostream_formatter {};
template <>
struct spdlog::fmt_lib::formatter<rad::Mat4> : spdlog::fmt_lib::ostream_formatter {};
std::ostream& operator<<(std::ostream& os, enum RTCError err);
template <>
struct spdlog::fmt_lib::formatter<enum RTCError> : spdlog::fmt_lib::ostream_formatter {};
