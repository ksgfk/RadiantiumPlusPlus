#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

using namespace Rad::Math;

namespace Rad {

class VolPath final : public SampleRenderer {
 public:
  VolPath(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
  }

  Spectrum Li(const RayDifferential& ray_, const Scene& scene, Sampler* sampler) const override {
    throw RadInvalidOperationException("not impl");
    // Ray ray = ray_;
    // Float eta = 1;
    // Spectrum throughput(1);
    // Spectrum result(0);
    // Medium* rayEnv = scene.GetGlobalMedium();
    // Int32 depth = 0;
    // bool isSpecularPath = true;
    // UInt32 channel = std::min((UInt32)(sampler->Next1D() * 3), 2u);
    // for (;; depth++) {
    //   //轮盘赌终止递归
    //   if (depth >= _rrDepth) {
    //     Float maxThroughput = throughput.MaxComponent();
    //     Float rr = std::min(maxThroughput * Sqr(eta), Float(0.95));
    //     if (sampler->Next1D() > rr) {
    //       break;
    //     }
    //     throughput *= Rcp(rr);
    //   }
    //   if (_maxDepth >= 0 && depth + 1 >= _maxDepth) {
    //     break;
    //   }
    //   SurfaceInteraction si{};
    //   bool isHit = scene.RayIntersect(ray, si);
    //   MediumInteraction mi{};
    //   if (rayEnv != nullptr) {
    //     mi = rayEnv->SampleInteraction(ray, sampler->Next1D(), channel);
    //     auto [tr, trPdf] = rayEnv->EvalTrAndPdf(mi, si);
    //     throughput *= tr / trPdf[channel];
    //   }
    //   if (mi.IsValid()) {
    //   } else {
    //   }
    // }
  }

  Float MisWeight(Float a, Float b) const {
    a *= a;
    b *= b;
    Float w = a / (a + b);
    return std::isfinite(w) ? w : 0;
  }

 private:
  Int32 _maxDepth;
  Int32 _rrDepth;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(VolPath, "vol_path");
