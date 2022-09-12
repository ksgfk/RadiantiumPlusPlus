#include <radiantium/renderer.h>

#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>
#include <radiantium/warp.h>
#include <radiantium/math_ext.h>

namespace rad {

/**
 * @brief 假设所有表面都是漫反射, 并从所有方向接收均匀的光照
 * L(x) = \int_\omega V * (cosTheta / PI) \mathrm{d}\omega
 * 
 */
class AO : public BlockSampleRenderer {
 public:
  AO(std::unique_ptr<World> world, Int32 spp, Int32 taskCount, bool isCosWeight)
      : BlockSampleRenderer(std::move(world), spp, taskCount), _isCosWeight(isCosWeight) {}

  Spectrum Li(const Ray& ray, const World* world, ISampler* sampler) override {
    SurfaceInteraction si;
    if (!world->RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    Vec3 wo;
    float pdf;
    if (_isCosWeight) {
      wo = warp::SquareToCosineHemisphere(sampler->Next2D());
      pdf = warp::SquareToCosineHemispherePdf(wo);
    } else {
      wo = warp::SquareToUniformHemisphere(sampler->Next2D());
      pdf = warp::SquareToUniformHemispherePdf();
    }
    Ray shadowRay = si.SpawnRay(si.ToWorld(wo));
    bool visibility = world->RayIntersect(shadowRay);
    Spectrum l(0);
    if (!visibility && Frame::CosTheta(wo) >= 0) {
      l = Frame::CosTheta(wo) / math::PI / pdf;
    }
    return l;
  }

 private:
  const bool _isCosWeight;
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
    bool isCosWeight = config->GetBool("cos_weight", true);
    return std::make_unique<AO>(context->MoveWorld(), spp, threadCount, isCosWeight);
  }
};
std::unique_ptr<IFactory> CreateAORenderer() {
  return std::make_unique<AOFactory>();
}
}  // namespace rad::factory
