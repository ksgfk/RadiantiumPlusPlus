#include <rad/offline/render/renderer.h>

#include <rad/offline/build/factory.h>
#include <rad/offline/spectrum.h>
#include <rad/offline/warp.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/render/interaction.h>
#include <rad/offline/render/scene.h>

namespace Rad {

/**
 * @brief Ambient Occlusion 环境光遮蔽
 * 假设所有表面都是漫反射, 并从所有方向接收均匀的光照
 * L(x) = ∫ V * (cosθ / PI) dω
 */
class AO final : public SampleRenderer {
 public:
  AO(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg) : SampleRenderer(ctx, std::move(scene), cfg) {}
  ~AO() noexcept override = default;

  Spectrum Li(const RayDifferential& ray, const Scene& scene, Sampler* sampler) const override {
    SurfaceInteraction si;
    if (!scene.RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    Vector3 wo = Warp::SquareToCosineHemisphere(sampler->Next2D());
    Float pdf = Warp::SquareToCosineHemispherePdf(wo);
    Ray shadowRay = si.SpawnRay(si.ToWorld(wo));
    bool visibility = scene.RayIntersect(shadowRay);
    Spectrum l(0);
    if (!visibility && Frame::CosTheta(wo) > 0) {
      l = Frame::CosTheta(wo) / Math::PI / pdf;
    }
    return l;
  }
};

class AOFactory final : public RendererFactory {
 public:
  AOFactory() : RendererFactory("ao") {}
  ~AOFactory() noexcept override = default;
  Unique<Renderer> Create(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg) const override {
    return std::make_unique<AO>(ctx, std::move(scene), cfg);
  }
};

Unique<RendererFactory> _FactoryCreateAOFunc_() {
  return std::make_unique<AOFactory>();
}

}  // namespace Rad
