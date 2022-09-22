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
    // Vector3 n = si.N;
    // Vector3 result = (n + Vector3(1, 1, 1)) * Float(0.5);
    // if (result.x() < -1 || result.y() < -1 || result.z() < -1) {
    //   Logger::Get()->warn("?");
    // }
    // return Spectrum(result);
    Bsdf* bsdf = si.Shape->GetBsdf();
    return bsdf->Eval({}, si, si.Wi);
  }
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(GBuffer, "gbuffer");
