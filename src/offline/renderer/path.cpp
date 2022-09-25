#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class Path final : public SampleRenderer {
 public:
  Path(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
  }

  Spectrum Li(const Ray& ray_, const Scene& scene, Sampler* sampler) const override {
#if 0
    Ray ray = ray_;
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    BsdfContext ctx{};
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      if (hitLight) {
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li));
        result += le;
      }
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
      if (bsr.Pdf <= 0) {
        break;
      } else {
        throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
        eta *= bsr.Eta;
      }
      ray = si.SpawnRay(si.ToWorld(bsr.Wo));
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
    }
    return result;
#endif

#if 0
    Ray ray = ray_;
    BsdfContext ctx{};
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    bool isSpecularPath = true;
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      if (hitLight && isSpecularPath) {
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li));
        result += le;
      }
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      if (si.Shape->IsLight()) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      {
        auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
        if (dsr.Pdf <= 0) {
          break;
        }
        Vector3 wo = si.ToLocal(dsr.Dir);
        Spectrum f = bsdf->Eval(ctx, si, wo);
        if (!scene.IsOcclude(si, dsr.P)) {
          Spectrum le(throughput.cwiseProduct(f).cwiseProduct(li) / dsr.Pdf);
          result += le;
        }
      }
      {
        auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
        if (bsr.Pdf <= 0) {
          break;
        } else {
          throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
          eta *= bsr.Eta;
        }
        ray = si.SpawnRay(si.ToWorld(bsr.Wo));
        isSpecularPath = bsr.HasType(BsdfType::Delta);
      }
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
    }
    return result;
#endif

#if 1
    Ray ray = ray_;
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    bool isSpecularPath = true;
    SurfaceInteraction prevSi{};
    Float prevBsdfPdf = 1;
    BsdfContext ctx{};
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      if (hitLight) {
        Float weight;
        if (isSpecularPath) {
          weight = 1;
        } else {
          DirectionSampleResult dsr = si.ToDsr(prevSi);
          Float lightPdf = scene.PdfLightDirection(*hitLight, prevSi, dsr);
          weight = MisWeight(prevBsdfPdf, lightPdf);
        }
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li) * weight);
        result += le;
      }
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      if (si.Shape->IsLight()) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      if (bsdf->HasAnyTypeExceptDelta()) {
        auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
        if (dsr.Pdf <= 0) {
          break;
        }
        Vector3 wo = si.ToLocal(dsr.Dir);
        Spectrum f = bsdf->Eval(ctx, si, wo);
        Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
        if (!scene.IsOcclude(si, dsr.P)) {
          Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, bsdfPdf);
          Spectrum le(throughput.cwiseProduct(f).cwiseProduct(li) * weight / dsr.Pdf);
          result += le;
        }
      }
      {
        auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
        if (bsr.Pdf <= 0) {
          break;
        } else {
          throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
          eta *= bsr.Eta;
        }
        ray = si.SpawnRay(si.ToWorld(bsr.Wo));
        prevBsdfPdf = bsr.Pdf;
        prevSi = si;
        isSpecularPath = bsr.HasType(BsdfType::Delta);
      }
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
    }
    return result;
#endif
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

RAD_FACTORY_RENDERER_DECLARATION(Path, "path");
