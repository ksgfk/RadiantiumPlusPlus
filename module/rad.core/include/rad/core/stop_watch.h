#pragma once

#include "types.h"

#include <chrono>

namespace Rad {

/**
 * @brief 工具类，用来计时
 */
class Stopwatch {
 public:
  /**
   * @brief 从开始到结束经过了多少毫秒, 如果计时器没结束则返回开始到现在经过了多少毫秒
   */
  inline Int64 ElapsedMilliseconds() const {
    Int64 result;
    if (_isStop) {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start);
      result = duration.count();
    } else {
      auto now = std::chrono::system_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start);
      result = duration.count();
    }
    return result;
  }

  inline void Start() {
    _isStop = false;
    _start = std::chrono::system_clock::now();
  }

  inline void Stop() {
    _stop = std::chrono::system_clock::now();
    _isStop = true;
  }

 private:
  std::chrono::system_clock::time_point _start{};
  std::chrono::system_clock::time_point _stop{};
  bool _isStop{true};
};

}  // namespace Rad
