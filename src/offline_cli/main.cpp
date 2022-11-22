#include <rad/offline/api.h>
#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>

using namespace Rad;

std::pair<Unique<Renderer>, Unique<LocationResolver>> Build(int argc, char** argv) {
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
    throw RadArgumentException("unknown cmd");
  }
  std::filesystem::path p(scenePath);
  nlohmann::json cfg;
  {
    std::ifstream cfgStream(p);
    cfg = nlohmann::json::parse(cfgStream);
  }
  ConfigNode cfgNode(&cfg);
  BuildContext ctx(
      p.parent_path().string(),
      p.filename().replace_extension().string(),
      &cfgNode);
  return ctx.Build();
}

int main(int argc, char** argv) {
  RadInit();
  try {
    auto [renderer, resolver] = Build(argc, argv);
    Logger::Get()->info("start rendering...");
    renderer->Start();
    renderer->Wait();
    renderer->SaveResult(*resolver);
    Logger::Get()->info("done. render used time: {}", renderer->ElapsedTime());
  } catch (const std::exception& e) {
    Logger::Get()->error("unhandled exception: {}", e.what());
  } catch (...) {
    Logger::Get()->error("unknown exception");
  }
  RadShutdown();
  return 0;
}
