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

    Vector2 uv = si.UV;
    return Spectrum(uv.x(), uv.y(), 0);
  }
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(GBuffer, "gbuffer");
