#include <rad/offline/render/sampler.h>

namespace Rad {

Sampler::Sampler(BuildContext* ctx, const ConfigNode& cfg) {
  _sampleCount = cfg.ReadOrDefault("sample_count", 16);
}

}  // namespace Rad
