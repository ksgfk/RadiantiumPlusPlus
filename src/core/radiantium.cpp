#include <radiantium/radiantium.h>

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
