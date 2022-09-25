#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

/**
 * @brief 直接照明
 * 只计算单一散射事件, 光路: Eye -> BSDF -> Light
 * 多重重要性采样(MIS), 可以将光源重要性采样与BSDF重要性采样结合, 显著方差
 * 在BSDF重要性采样时, 如果不能采样到连接光源的路径, 就毫无意义, 整体路径没有任何贡献
 * 在光源重要性采样时, 在许多情况下是有效的, 但如果采样点与表面连接的路径不在BSDF的lobe内,
 * 也会造成无效路径, 例如很光滑的微表面, delta路径, 尤其是后者根本无法使用光源重要性来评估
 * MIS将两种采样策略得到的样本按权重叠加, 抑制pdf较低的样本, 放大pdf较高样本所占权重, 可以有效减少方差
 */
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
    // auto [light, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
    auto [lightIndex, selectPdf] = scene.SampleLight(sampler->Next1D());
    const Light* light = scene.GetLight(lightIndex);
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
    result += Spectrum(f.cwiseProduct(li) / dsr.Pdf / selectPdf);
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

#if 0
    Spectrum result(0);
    SurfaceInteraction si;
    bool anyHit = scene.RayIntersect(ray, si);
    if (!anyHit) {
      return result;
    }
    if (si.Shape->IsLight()) {  //直接击中光源
      return si.Shape->GetLight()->Eval(si);
    }
    BsdfContext ctx{};
    Bsdf* bsdf = si.Shape->GetBsdf();
    //光源重要性采样
    for (size_t i = 0; i < _lightSamples; i++) {
      auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
      if (dsr.Pdf <= 0) {
        continue;
      }
      //可见性测试
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
    // BSDF重要性采样
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
      if (!bsdfSi.Shape->IsLight()) {  //击中光源才能对路径有贡献
        continue;
      }
      Light* light = bsdfSi.Shape->GetLight();
      bool isDelta = bsr.HasType(BsdfType::Delta);
      DirectionSampleResult dsr = bsdfSi.ToDsr(si);
      Float lightPdf = isDelta ? 0 : light->PdfDirection(si, dsr);
      Spectrum li = light->Eval(bsdfSi);
      Float mis = MisWeight(bsr.Pdf * _fracBsdf, lightPdf * _fracLight) * _weightBsdf;
      auto le = f.cwiseProduct(li) * mis / bsr.Pdf;
      result += Spectrum(le);
    }
    return result;
#endif
  }

  Float MisWeight(Float a, Float b) const {
    a *= a;  // power heuristic
    b *= b;
    Float w = a / (a + b);
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
