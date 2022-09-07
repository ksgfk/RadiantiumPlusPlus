#include <radiantium/renderer.h>

#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

namespace rad {

class AO : public BlockSampleRenderer {
 public:
  AO(std::unique_ptr<World> world, Int32 spp, Int32 taskCount, bool isCosWeight)
      : BlockSampleRenderer(std::move(world), spp, taskCount), _isCosWeight(isCosWeight) {}

  Spectrum Li(const Ray& ray, const World* world, ISampler* sampler) override {
    SurfaceInteraction si;
    if (!world->RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    Spectrum n(si.Shading.N.cwiseAbs());
    //Spectrum n(si.N.cwiseAbs());
    return n;
  }

 private:
  bool _isCosWeight;
};

}  // namespace rad

namespace rad::factory {
class AOFactory : public IRendererFactory {
 public:
  ~AOFactory() noexcept override {}
  std::string UniqueId() const override { return "ao"; }
  std::unique_ptr<IRenderer> Create(BuildContext* context, const IConfigNode* config) const override {
    Int32 spp = config->GetInt32("spp", 32);
    Int32 threadCount = config->GetInt32("thread_count", -1);
    return std::make_unique<AO>(context->MoveWorld(), spp, threadCount, false);
  }
};
std::unique_ptr<IFactory> CreateAORenderer() {
  return std::make_unique<AOFactory>();
}
}  // namespace rad::factory
