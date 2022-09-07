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
  rad::path p = scenePath;
  auto config = rad::config::CreateJsonConfig(p);
  rad::BuildContext ctx;
  ctx.SetConfig(std::move(config));
  ctx.SetLocationResolver(rad::LocationResolver{p.parent_path()});
  ctx.SetOutputName(p.filename().replace_extension().string());
  ctx.Build();
  return ctx.Result();
}

int main(int argc, char** argv) {
  rad::logger::InitLogger();
  auto [renderer, output] = GetRenderer(argc, argv);
  rad::logger::GetLogger()->info("start rendering...");
  renderer->Start();
  renderer->Wait();
  renderer->SaveResult(output);
  rad::logger::ShutdownLogger();
}
