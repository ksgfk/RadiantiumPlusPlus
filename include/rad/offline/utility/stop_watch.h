#pragma once

#include "../common.h"

#include <chrono>

namespace Rad {
/**
 * @brief 计时器
 *
 */
class RAD_EXPORT_API Stopwatch {
 public:
  /**
   * @brief 从开始到结束经过了多少毫秒, 如果计时器没结束则返回开始到现在经过了多少毫秒
   */
  Int64 ElapsedMilliseconds() const;

  void Start();
  void Stop();

 private:
  std::chrono::system_clock::time_point _start;
  std::chrono::system_clock::time_point _stop;
  bool _isStop;
};

}  // namespace Rad
