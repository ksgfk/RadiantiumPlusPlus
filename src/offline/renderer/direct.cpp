#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class Direct final : public SampleRenderer {
 public:
  Direct(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {
    UInt32 shadingSamples = cfg.ReadOrDefault("shading_samples", 1);
    _lightSamples = cfg.ReadOrDefault("light_samples", shadingSamples);
    _bsdfSamples = cfg.ReadOrDefault("bsdf_samples", shadingSamples);

    UInt32 sum = _lightSamples + _bsdfSamples;
    _weightBsdf = Float(1.0) / _bsdfSamples;
    _weightLight = Float(1.0) / _lightSamples;
    _fracBsdf = _bsdfSamples / Float(sum);
    _fracLight = _lightSamples / Float(sum);
  }
  ~Direct() noexcept override = default;

  Spectrum Li(const Ray& ray, const Scene& scene, Sampler* sampler) const override {
#if 0
    Spectrum result(0);
    SurfaceInteraction si;
    bool anyHit = scene.RayIntersect(ray, si);
    if (!anyHit) {
      return result;
    }
    if (si.Shape->IsLight()) {
      return si.Shape->GetLight()->Eval(si);
    }

    // auto [light, dsr, li] = scene.SampleLightDirection(si, sampler->Next2D());
    auto [index, weight, _] = scene.SampleLight(sampler->Next1D());
    const Light* light = scene.GetLight(index);
    auto [dsr, li] = light->SampleDirection(si, sampler->Next2D());
    if (dsr.Pdf <= 0) {
      return result;
    }
    if (scene.IsOcclude(si, dsr.P)) {
      return result;
    }
    BsdfContext ctx{};
    Bsdf* bsdf = si.Shape->GetBsdf();
    Spectrum f = bsdf->Eval(ctx, si, si.ToLocal(dsr.Dir));
    result += Spectrum(f.cwiseProduct(li) * weight / dsr.Pdf);
    return result;
#endif

#if 0
    Spectrum result(0);
    SurfaceInteraction si;
    bool anyHit = scene.RayIntersect(ray, si);
    if (!anyHit) {
      return result;
    }
    if (si.Shape->IsLight()) {
      return si.Shape->GetLight()->Eval(si);
    }

    BsdfContext ctx{};
    Bsdf* bsdf = si.Shape->GetBsdf();

    auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
    if (bsr.Pdf <= 0) {
      return result;
    }
    SurfaceInteraction lightSi;
    if (!scene.RayIntersect(si.SpawnRay(si.ToWorld(bsr.Wo)), lightSi)) {
      return result;
    }
    if (!lightSi.Shape->IsLight()) {
      return result;
    }
    Light* light = lightSi.Shape->GetLight();
    Spectrum li = light->Eval(lightSi);
    Spectrum le = Spectrum(f.cwiseProduct(li) / bsr.Pdf);
    result += le;
    return result;
#endif

#if 1
    Spectrum result(0);
    SurfaceInteraction si;
    bool anyHit = scene.RayIntersect(ray, si);
    if (!anyHit) {
      return result;
    }
    if (si.Shape->IsLight()) {
      return si.Shape->GetLight()->Eval(si);
    }
    BsdfContext ctx{};
    Bsdf* bsdf = si.Shape->GetBsdf();
    for (size_t i = 0; i < _lightSamples; i++) {
      auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
      if (dsr.Pdf <= 0) {
        continue;
      }
      if (scene.IsOcclude(si, dsr.P)) {
        continue;
      }
      Vector3 wo = si.ToLocal(dsr.Dir);
      Spectrum f = bsdf->Eval(ctx, si, wo);
      Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
      if (bsdfPdf <= 0) {
        continue;
      }
      Float mis = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf * _fracLight, bsdfPdf * _fracBsdf) * _weightLight;
      auto le = f.cwiseProduct(li) * mis / dsr.Pdf;
      result += Spectrum(le);
    }
    for (size_t i = 0; i < _bsdfSamples; i++) {
      auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
      if (bsr.Pdf <= 0) {
        continue;
      }
      SurfaceInteraction bsdfSi;
      bool bsdfHit = scene.RayIntersect(si.SpawnRay(si.ToWorld(bsr.Wo)), bsdfSi);
      if (!bsdfHit) {
        continue;
      }
      if (!bsdfSi.Shape->IsLight()) {
        continue;
      }
      Light* light = bsdfSi.Shape->GetLight();
      bool isDelta = bsr.HasType(BsdfType::Delta);
      DirectionSampleResult dsr = bsdfSi.ToDsr(si);
      Float lightPdf = isDelta ? 0 : light->PdfDirection(si, dsr);
      if (lightPdf <= 0) {
        continue;
      }
      Spectrum li = light->Eval(bsdfSi);
      Float mis = MisWeight(bsr.Pdf * _fracBsdf, lightPdf * _fracLight) * _weightBsdf;
      auto le = f.cwiseProduct(li) * mis / bsr.Pdf;
      result += Spectrum(le);
    }
    return result;
#endif
  }

  Float MisWeight(Float pdf_a, Float pdf_b) const {
    pdf_a *= pdf_a;
    pdf_b *= pdf_b;
    Float w = pdf_a / (pdf_a + pdf_b);
    return std::isfinite(w) ? w : 0;
  }

 private:
  UInt32 _lightSamples;
  UInt32 _bsdfSamples;
  Float _fracBsdf, _fracLight;
  Float _weightBsdf, _weightLight;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(Direct, "direct");
