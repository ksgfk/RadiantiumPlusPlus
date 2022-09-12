#include <radiantium/logger.h>
#include <radiantium/renderer.h>
#include <radiantium/location_resolver.h>
#include <radiantium/config_node.h>
#include <radiantium/build_context.h>

#include <string>

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
  if(renderer == nullptr) {
    rad::logger::GetLogger()->warn("no valid scene. exit");
    return 0;
  }
  rad::logger::GetLogger()->info("start rendering...");
  renderer->Start();
  renderer->Wait();
  rad::logger::GetLogger()->info("render used time: {} ms", renderer->ElapsedTime());
  renderer->SaveResult(output);
  rad::logger::ShutdownLogger();
}
