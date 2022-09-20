#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class GBuffer final : public SampleRenderer {
 public:
  GBuffer(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {}
  ~GBuffer() noexcept override = default;

  Spectrum Li(const Ray& ray, const Scene& scene, Sampler* sampler) const override {
    SurfaceInteraction si;
    if (!scene.RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    Vector3 n = si.N;
    auto result = (n + Vector3(1, 1, 1)) * Float(0.5);
    return Spectrum(result);
  }
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(GBuffer, "gbuffer");
