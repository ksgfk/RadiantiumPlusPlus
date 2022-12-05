#pragma once

#include "macros.h"

namespace Rad {

#if defined(__cplusplus)
extern "C" {
#endif
RAD_EXPORT_API void RadCoreInit();
RAD_EXPORT_API void RadCoreShutdown();
#if defined(__cplusplus)
}
#endif

}  // namespace Rad
