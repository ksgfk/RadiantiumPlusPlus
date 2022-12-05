#include <rad/offline/render/sampler.h>

#include <rad/offline/build/factory.h>

#include <random>
#include <chrono>

namespace Rad {

/**
 * @brief 独立样本的采样器，实际上就是均匀随机数发生器
 */
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
    Float s1 = _dist(_engine);
    Float s2 = _dist(_engine);
    return Vector2(s1, s2);
  }
  Vector3 Next3D() override {
    Float s1 = _dist(_engine);
    Float s2 = _dist(_engine);
    Float s3 = _dist(_engine);
    return Vector3(s1, s2, s3);
  }

 private:
  std::mt19937 _engine;
  std::uniform_real_distribution<Float> _dist;
};

class IndependentFactory final : public SamplerFactory {
 public:
  IndependentFactory() : SamplerFactory("independent") {}
  ~IndependentFactory() noexcept override = default;
  Unique<Sampler> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Independent>(ctx, cfg);
  }
};

Unique<SamplerFactory> _FactoryCreateIndependentFunc_() {
  return std::make_unique<IndependentFactory>();
}

}  // namespace Rad
