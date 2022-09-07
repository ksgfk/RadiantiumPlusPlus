#pragma once

#include "radiantium.h"

#include <chrono>

namespace rad {

class Stopwatch {
 public:
  Int64 ElapsedMilliseconds() const;

  void Start();
  void Stop();

 private:
  std::chrono::system_clock::time_point _start;
  std::chrono::system_clock::time_point _stop;
  bool _isStop;
};

}  // namespace rad
