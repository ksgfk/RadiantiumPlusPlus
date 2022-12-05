#pragma once

#include <cstdint>
#include <cstddef>  //std::byte
#include <cmath>
#include <memory>
#include <stdexcept>  //std::runtime_error

#include <Eigen/Dense>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include "macros.h"

namespace Rad {

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

using Vector2f = Eigen::Vector2<Float32>;
using Vector3f = Eigen::Vector3<Float32>;
using Vector4f = Eigen::Vector4<Float32>;
using Matrix3f = Eigen::Matrix3<Float32>;
using Matrix4f = Eigen::Matrix4<Float32>;
using BoundingBox2f = Eigen::AlignedBox<Float32, 2>;
using BoundingBox3f = Eigen::AlignedBox<Float32, 3>;

using Vector2i = Eigen::Vector2<Int32>;
using Vector3i = Eigen::Vector3<Int32>;
using Vector4i = Eigen::Vector4<Int32>;

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

class RadNotImplementedException : public RadException {
 public:
  template <typename... Args>
  RadNotImplementedException(const char* fmt, const Args&... args)
      : RadException("RadNotImplementedException", fmt, args...) {}
};

class RadNotSupportedException : public RadException {
 public:
  template <typename... Args>
  RadNotSupportedException(const char* fmt, const Args&... args)
      : RadException("RadNotSupportedException", fmt, args...) {}
};

}  // namespace Rad

template <>
struct spdlog::fmt_lib::formatter<Rad::Vector2f> {
  template <typename FormatContext>
  auto format(const Rad::Vector2f& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}>", v[0], v[1]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector3f> {
  template <typename FormatContext>
  auto format(const Rad::Vector3f& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector4f> {
  template <typename FormatContext>
  auto format(const Rad::Vector4f& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}, {}>", v[0], v[1], v[2], v[3]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Matrix3f> : spdlog::fmt_lib::ostream_formatter {};
template <>
struct spdlog::fmt_lib::formatter<Rad::Matrix4f> : spdlog::fmt_lib::ostream_formatter {};


template <>
struct spdlog::fmt_lib::formatter<Rad::Vector2i> {
  template <typename FormatContext>
  auto format(const Rad::Vector2i& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}>", v[0], v[1]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector3i> {
  template <typename FormatContext>
  auto format(const Rad::Vector3i& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}>", v[0], v[1], v[2]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
template <>
struct spdlog::fmt_lib::formatter<Rad::Vector4i> {
  template <typename FormatContext>
  auto format(const Rad::Vector4i& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    return spdlog::fmt_lib::format_to(ctx.out(), "<{}, {}, {}, {}>", v[0], v[1], v[2], v[3]);
  }
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
};
