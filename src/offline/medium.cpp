#include <rad/offline/render/medium.h>

#include <rad/offline/build.h>

namespace Rad {

Medium::Medium(BuildContext* ctx, const ConfigNode& cfg) {
  ConfigNode phaseNode;
  if (cfg.TryRead("phase", phaseNode)) {
    std::string type = phaseNode.Read<std::string>("type");
    PhaseFunctionFactory* factory = ctx->GetFactory<PhaseFunctionFactory>(type);
    _phaseFunction = factory->Create(ctx, phaseNode);
  } else {
    throw RadArgumentException("medium must have phase function");
  }
}

}  // namespace Rad
