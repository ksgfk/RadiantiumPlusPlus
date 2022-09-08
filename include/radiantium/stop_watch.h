#pragma once

#include "radiantium.h"

#include <chrono>

namespace rad {
/**
 * @brief 计时器
 *
 */
class Stopwatch {
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

}  // namespace rad
