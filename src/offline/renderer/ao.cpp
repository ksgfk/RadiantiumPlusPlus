#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class AO final : public SampleRenderer {
 public:
  AO(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {}
  ~AO() noexcept override = default;

  Spectrum Li(const Ray& ray, const Scene& scene, Sampler* sampler) const override {
    SurfaceInteraction si;
    if (!scene.RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    Vector3 wo = Warp::SquareToCosineHemisphere(sampler->Next2D());
    float pdf = Warp::SquareToCosineHemispherePdf(wo);
    Ray shadowRay = si.SpawnRay(si.ToWorld(wo));
    bool visibility = scene.RayIntersect(shadowRay);
    Spectrum l(0);
    if (!visibility && Frame::CosTheta(wo) > 0) {
      l = Frame::CosTheta(wo) / Math::PI / pdf;
    }
    return l;
  }
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(AO, "ao");
