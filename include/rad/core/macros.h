#pragma once

namespace Rad {

#if defined(RAD_BUILD_SHARED)
#if defined(_WIN32)
#define RAD_EXPORT_API __declspec(dllexport)
#define RAD_IMPORT_API __declspec(dllimport)
#else
#define RAD_EXPORT_API __attribute__((visibility("default")))
#define RAD_IMPORT_API
#endif
#else
#define RAD_EXPORT_API
#define RAD_IMPORT_API
#endif

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

}
