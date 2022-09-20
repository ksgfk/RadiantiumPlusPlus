#include <rad/offline/render/sampler.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <random>
#include <chrono>

namespace Rad {

class Independent final : public Sampler {
 public:
  Independent(BuildContext* ctx, const ConfigNode& cfg) : Sampler(ctx, cfg) {
    _seed = cfg.ReadOrDefault(
        "seed",
        (UInt32)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    _engine = std::mt19937(_seed);
  }
  ~Independent() noexcept override = default;

  Unique<Sampler> Clone(UInt32 seed) const override {
    auto result = std::make_unique<Independent>(nullptr, ConfigNode{});
    result->SetSeed(seed);
    return std::move(result);
  }

  void SetSeed(UInt32 seed) override {
    _engine = std::mt19937(seed);
    _seed = seed;
  }

  Float Next1D() override {
    return _dist(_engine);
  }
  Vector2 Next2D() override {
    Float s1 = Next1D();
    Float s2 = Next1D();
    return Vector2(s1, s2);
  }

 private:
  std::mt19937 _engine;
  std::uniform_real_distribution<Float> _dist;
};

}  // namespace Rad

RAD_FACTORY_SAMPLER_DECLARATION(Independent, "independent");
