#pragma once

#include <iostream>

#include "types.h"
#include <fmt/format.h>

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pthread.h>
#else
#warning "unknwon platform"
#endif

namespace Rad {

/**
 * @brief 工具类，在控制台打印一个进度条
 */
class ConsoleProgressBar {
 public:
  inline ConsoleProgressBar() {
#if defined(_MSC_VER)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE && h != nullptr) {
      CONSOLE_SCREEN_BUFFER_INFO bufferInfo{};
      GetConsoleScreenBufferInfo(h, &bufferInfo);
      _lineLength = bufferInfo.dwSize.X;
    }
#elif defined(__linux__)
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) >= 0) {
      _lineLength = w.ws_col;
    }
#else
    _lineLength = 80;
#endif
    _line = std::string(_lineLength, ' ');
    _lineLength -= 15;
  }

  /**
   * @brief 更新进度条
   *
   * @param elapsed 剩余时间（单位：毫秒）
   * @param all 总任务量
   * @param complete 已完成的任务量
   */
  inline void Draw(Int64 elapsed, UInt64 all, UInt64 complete) {
    float progress = (float)complete / all;
    int pos = (int)(_lineLength * progress);
    _line.clear();
    _line += '[';
    for (int i = 1; i < _lineLength; i++) {
      if (i < pos) {
        _line += '=';
      } else if (i == pos) {
        _line += '>';
      } else {
        _line += ' ';
      }
    }
    _line += ']';
    _line += fmt::format("{:.2f}%", progress * 100);
    _line += ' ';
    _line += TimeStr((float)elapsed, false);
    std::cout << '\r' << _line;
    std::cout.flush();
  }

 private:
  static inline std::string TimeStr(float value, bool precise) {
    struct Order {
      float factor;
      const char* suffix;
    };
    const Order orders[] = {{0, "ms"}, {1000, "s"}, {60, "m"}, {60, "h"}, {24, "d"}, {7, "w"}, {52.1429f, "y"}};
    if (std::isnan(value)) {
      return "NaN";
    } else if (std::isinf(value)) {
      return "Inf";
    } else if (value < 0) {
      return "-" + TimeStr(-value, precise);
    }
    int i = 0;
    for (i = 0; i < 6 && value > orders[i + 1].factor; ++i) {
      value /= orders[i + 1].factor;
    }
    return fmt::format(precise ? "{:.2f}{}" : "{:.0f}{}", value, orders[i].suffix);
  }

  int _lineLength;
  std::string _line;
};

}  // namespace Rad
