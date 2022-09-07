#include <radiantium/stop_watch.h>

namespace rad {

Int64 Stopwatch::ElapsedMilliseconds() const {
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

void Stopwatch::Start() {
  _isStop = false;
  _start = std::chrono::system_clock::now();
}

void Stopwatch::Stop() {
  _stop = std::chrono::system_clock::now();
  _isStop = true;
}

}  // namespace rad
