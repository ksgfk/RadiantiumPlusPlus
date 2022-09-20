#pragma once

#include "../common.h"
#include "../build/config_node.h"

namespace Rad {

class Sampler {
 public:
  inline Sampler(BuildContext* ctx, const ConfigNode& cfg) {
    _sampleCount = cfg.ReadOrDefault("sample_count", 16);
  }
  virtual ~Sampler() noexcept = default;

  UInt32 SampleCount() const { return _sampleCount; }

  virtual Unique<Sampler> Clone(UInt32 seed) const = 0;

  UInt32 GetSeed() const { return _seed; }
  virtual void SetSeed(UInt32 seed) = 0;

  virtual Float Next1D() = 0;
  virtual Vector2 Next2D() = 0;

 protected:
  UInt32 _sampleCount;
  UInt32 _seed;
};

}  // namespace Rad
