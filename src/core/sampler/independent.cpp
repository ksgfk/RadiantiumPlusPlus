#include <radiantium/sampler.h>

#include <radiantium/factory.h>
#include <radiantium/config_node.h>
#include <radiantium/build_context.h>

#include <random>

namespace rad {

class IndependentSampler : public ISampler {
 public:
  IndependentSampler() {
    _seed = static_cast<UInt32>(Clock::now().time_since_epoch().count());
    _engine = std::mt19937(_seed);
  }
  IndependentSampler(UInt32 seed) {
    _seed = seed;
    _engine = std::mt19937(_seed);
  }

  ~IndependentSampler() noexcept override {}

  UInt32 Seed() const override { return _seed; }

  std::unique_ptr<ISampler> Clone() const override {
    return std::make_unique<IndependentSampler>(_seed);
  }

  Float Next1D() override {
    return _dist(_engine);
  }

  Vec2 Next2D() override {
    Float s1 = Next1D();
    Float s2 = Next1D();
    return Vec2(s1, s2);
  }

  void SetSeed(UInt32 seed) override {
    _seed = seed;
    _engine = std::mt19937(seed);
  }

 private:
  UInt32 _seed;
  std::mt19937 _engine;
  std::uniform_real_distribution<Float> _dist;
};

}  // namespace rad

namespace rad::factory {
class IndependentSamplerFactory : public ISamplerFactory {
 public:
  ~IndependentSamplerFactory() noexcept override {}
  std::string UniqueId() const override { return "independent"; }
  std::unique_ptr<ISampler> Create(const BuildContext* context, const IConfigNode* config) const override {
    std::unique_ptr<ISampler> result;
    if (config->HasProperty("seed")) {
      UInt32 seed = static_cast<UInt32>(config->GetInt32("seed", 114514));
      result = std::make_unique<IndependentSampler>(seed);
    } else {
      result = std::make_unique<IndependentSampler>();
    }
    return result;
  }
};
std::unique_ptr<IFactory> CreateIndependentSamplerFactory() {
  return std::make_unique<IndependentSamplerFactory>();
}
}  // namespace rad::factory
