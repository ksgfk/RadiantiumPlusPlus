#include <radiantium/logger.h>
#include <radiantium/renderer.h>
#include <radiantium/location_resolver.h>
#include <radiantium/config_node.h>
#include <radiantium/build_context.h>

#include <string>
#include <iostream>

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pthread.h>
#else
#warning "unknwon platform"
#endif

class Progress {
 public:
  Progress() {
#if defined(_MSC_VER)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE && h != nullptr) {
      CONSOLE_SCREEN_BUFFER_INFO bufferInfo{};
      GetConsoleScreenBufferInfo(h, &bufferInfo);
      _lineLength = bufferInfo.dwSize.X;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) >= 0)
      _line = w.ws_col;
#endif
    _lineLength = 80;
    _line = std::string(_lineLength, ' ');
    _lineLength -= 10;
  }

  void Draw(rad::Int64 elapsed, rad::UInt32 all, rad::UInt32 complete) {
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

  std::string TimeStr(float value, bool precise) {
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

std::pair<std::unique_ptr<rad::IRenderer>, rad::DataWriter> GetRenderer(int argc, char** argv) {
  std::string scenePath;
  for (int i = 0; i < argc;) {
    std::string cmd(argv[i]);
    if (cmd == "--scene") {
      scenePath = std::string(argv[i + 1]);
      i += 2;
    } else {
      i++;
    }
  }
  if (scenePath.empty()) {
    return std::make_pair<std::unique_ptr<rad::IRenderer>, rad::DataWriter>(nullptr, {});
  }
  rad::path p = scenePath;
  auto config = rad::config::CreateJsonConfig(p);
  rad::BuildContext ctx;
  ctx.SetConfig(std::move(config));
  ctx.SetWorkPath(p.parent_path().string());
  ctx.SetOutputName(p.filename().replace_extension().string());
  ctx.Build();
  return ctx.Result();
}

int main(int argc, char** argv) {
  rad::logger::InitLogger();
  auto [renderer, output] = GetRenderer(argc, argv);
  if (renderer == nullptr) {
    rad::logger::GetLogger()->warn("no valid scene. exit");
    return 0;
  }
  auto& render = *renderer.get();
  rad::logger::GetLogger()->info("start rendering...");
  std::thread barThread([&render]() {
    Progress bar;
    while (!render.IsComplete()) {
      bar.Draw(render.ElapsedTime(), render.AllTaskCount(), render.CompletedTaskCount());
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
#if defined(_MSC_VER)
  SetThreadPriority(barThread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(__linux__)
  sched_param sch;
  int policy;
  pthread_getschedparam(t1.native_handle(), &policy, &sch);
  sch.sched_priority = -20;
  pthread_setschedparam(t1.native_handle(), SCHED_RR, &sch)
#endif
  barThread.detach();
  renderer->Start();
  renderer->Wait();
  std::cout << std::endl;
  rad::logger::GetLogger()->info("render used time: {} ms", renderer->ElapsedTime());
  renderer->SaveResult(output);
  rad::logger::ShutdownLogger();
}
