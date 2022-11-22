#pragma once

#include <cstdint>
#include <cstddef>  //std::byte
#include <cmath>
#include <stdexcept>  //std::runtime_error
#include <memory>

#include <Eigen/Dense>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <embree3/rtcore.h>

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

using Vector2 = Eigen::Vector2<Float>;
using Vector3 = Eigen::Vector3<Float>;
using Vector4 = Eigen::Vector4<Float>;
using Matrix3 = Eigen::Matrix3<Float>;
using Matrix4 = Eigen::Matrix4<Float>;
using BoundingBox2 = Eigen::AlignedBox<Float, 2>;
using BoundingBox3 = Eigen::AlignedBox<Float, 3>;

template <typename T>
using MatrixX = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

template <typename T>
using Unique = std::unique_ptr<T>;
template <typename T>
using Share = std::shared_ptr<T>;

class RadException : public std::runtime_error {
 public:
  template <typename... Args>
  RadException(const char* fmt, const Args&... args) : RadException("RadException", fmt, args...) {}

  template <typename... Args>
  RadException(const char* eName, const char* fmt, const Args&... args)
      : std::runtime_error(fmt::format("{}: {}", eName, fmt::format(fmt, args...))) {}
};

class RadOutOfRangeException : public RadException {
 public:
  template <typename... Args>
  RadOutOfRangeException(const char* fmt, const Args&... args)
      : RadException("RadOutOfRangeException", fmt, args...) {}
};

class RadArgumentException : public RadException {
 public:
  template <typename... Args>
  RadArgumentException(const char* fmt, const Args&... args)
      : RadException("RadArgumentException", fmt, args...) {}
};

class RadInvalidCastException : public RadException {
 public:
  template <typename... Args>
  RadInvalidCastException(const char* fmt, const Args&... args)
      : RadException("RadInvalidCastException", fmt, args...) {}
};

class RadInvalidOperationException : public RadException {
 public:
  template <typename... Args>
  RadInvalidOperationException(const char* fmt, const Args&... args)
      : RadException("RadInvalidOperationException", fmt, args...) {}
};

class RadFileNotFoundException : public RadException {
 public:
  template <typename... Args>
  RadFileNotFoundException(const char* fmt, const Args&... args)
      : RadException("RadFileNotFoundException", fmt, args...) {}
};

}  // namespace Rad

// 照着抄的 https://fmt.dev/latest/api.html#format-api
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
std::ostream& operator<<(std::ostream& os, enum RTCError err);
template <>
struct spdlog::fmt_lib::formatter<enum RTCError> : spdlog::fmt_lib::ostream_formatter {};
