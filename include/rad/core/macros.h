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

}