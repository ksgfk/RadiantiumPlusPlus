#include <radiantium/renderer.h>

#include <radiantium/light.h>
#include <radiantium/math_ext.h>
#include <radiantium/frame.h>

#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

namespace rad {

class Direct : public BlockSampleRenderer {
 public:
  Direct(std::unique_ptr<World> world, Int32 spp, Int32 taskCount)
      : BlockSampleRenderer(std::move(world), spp, taskCount) {
  }

  Spectrum Li(const Ray& ray, const World* world, ISampler* sampler) override {
    SurfaceInteraction si;
    if (!world->RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    const auto& hit = world->GetEntity(si.EntityIndex);
    if (hit.HasLight()) {
      const ILight* light = hit.Light();
      return light->Eval(si);
    }
    if (!hit.HasBsdf()) {
      return Spectrum(Float(0.5), Float(0), Float(0.5));
    }
    const IBsdf* bsdf = hit.Bsdf();
    BsdfContext ctx{};
    auto [bsr, f] = bsdf->Sample(
        ctx,
        si, sampler->Next1D(), sampler->Next2D());
    if (bsr.Pdf <= 0) {
      return Spectrum(0);
    }
    Ray next = si.SpawnRay(si.ToWorld(bsr.Wo));
    if (!world->RayIntersect(next, si)) {
      return Spectrum(0);
    }
    const auto& nextHit = world->GetEntity(si.EntityIndex);
    if (nextHit.HasLight()) {
      const ILight* light = nextHit.Light();
      Spectrum li = light->Eval(si);
      Vec3 lig = li.cwiseProduct(f);
      Vec3 result = lig / bsr.Pdf;
      Spectrum cal = Spectrum(result);
      return cal;
    } else {
      return Spectrum(0);
    }
  }
};

}  // namespace rad

namespace rad::factory {
class DirectFactory : public IRendererFactory {
 public:
  ~DirectFactory() noexcept override {}
  std::string UniqueId() const override { return "direct"; }
  std::unique_ptr<IRenderer> Create(BuildContext* context, const IConfigNode* config) const override {
    Int32 spp = config->GetInt32("spp", 32);
    Int32 threadCount = config->GetInt32("thread_count", -1);
    return std::make_unique<Direct>(context->MoveWorld(), spp, threadCount);
  }
};
std::unique_ptr<IFactory> CreateDirectRenderer() {
  return std::make_unique<DirectFactory>();
}
}  // namespace rad::factory
