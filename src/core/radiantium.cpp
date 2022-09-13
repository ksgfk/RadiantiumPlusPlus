#include <radiantium/radiantium.h>

#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace rad {

void* AlignedAlloc(std::size_t alignment, std::size_t size) {
#if defined(_MSC_VER)
  return _aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

void AlignedFree(void* ptr) {
#if defined(_MSC_VER)
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

}  // namespace rad

std::ostream& operator<<(std::ostream& os, enum RTCError err) {
  switch (err) {
    case RTC_ERROR_NONE:
      os << "None";
      break;
    case RTC_ERROR_UNKNOWN:
      os << "Unknown";
      break;
    case RTC_ERROR_INVALID_ARGUMENT:
      os << "InvalidArgument";
      break;
    case RTC_ERROR_INVALID_OPERATION:
      os << "InvalidOperation";
      break;
    case RTC_ERROR_OUT_OF_MEMORY:
      os << "OutOfMemory";
      break;
    case RTC_ERROR_UNSUPPORTED_CPU:
      os << "UnsupportedCPU";
      break;
    case RTC_ERROR_CANCELLED:
      os << "Cancelled";
      break;
  }
  return os;
}
