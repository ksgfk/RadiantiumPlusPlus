#include <rad/core/common.h>
#include <rad/core/logger.h>
#include <rad/core/config_node.h>
#include <rad/core/console_progress_bar.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/render/renderer.h>

int main(int argc, char** argv) {
  Rad::RadCoreInit();
  try {
    Rad::Unique<Rad::LocationResolver> resolver;
    Rad::Unique<Rad::Renderer> renderer;
    {
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
        throw Rad::RadArgumentException("should input cmd like \"--scene <scene.json>\"");
      }
      std::filesystem::path p(scenePath);
      if (!std::filesystem::exists(p)) {
        throw Rad::RadArgumentException("cannot open file: {}", scenePath);
      }
      nlohmann::json cfg;
      {
        std::ifstream cfgStream(p);
        if (!cfgStream.is_open()) {
          throw Rad::RadArgumentException("cannot open file: {}", scenePath);
        }
        cfg = nlohmann::json::parse(cfgStream);
      }
      Rad::BuildContext ctx{};
      ctx.SetFromJson(cfg);
      ctx.SetDefaultFactoryManager();
      ctx.SetDefaultAssetManager(p.parent_path().string());
      renderer = ctx.Build();
      resolver = std::make_unique<Rad::LocationResolver>(ctx.GetAssetManager().GetLocationResolver());
      resolver->SetSaveName(p.filename().replace_extension().string());
    }
    Rad::Logger::Get()->info("start rendering...");
    std::thread barThread([&renderer]() {
      Rad::ConsoleProgressBar bar{};
      while (!renderer->IsComplete()) {
        bar.Draw(renderer->ElapsedTime(), renderer->AllTaskCount(), renderer->CompletedTaskCount());
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      std::cout << std::endl;
    });
    renderer->Start();
    renderer->Wait();
    renderer->SaveResult(*resolver);
    barThread.join();
    Rad::Logger::Get()->info("done. render used time: {} ms", renderer->ElapsedTime());
  } catch (const std::exception& e) {
    Rad::Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Rad::Logger::Get()->error("unknown exception");
  }
  Rad::RadCoreShutdown();
}
