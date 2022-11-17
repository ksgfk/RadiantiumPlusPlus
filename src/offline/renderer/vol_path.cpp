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
    _rrDepth = cfg.ReadOrDefault("rr_depth", 7);
  }

  Spectrum Li(const RayDifferential& ray_, const Scene& scene, Sampler* sampler) const override {
    // RayDifferential ray = ray_;
    // const Medium* rayMedium = scene.GetGlobalMedium();
    // Spectrum throughput(1);
    // Spectrum result(0);
    // Float eta = 1;
    // Int32 depth = 0;
    // bool isSpecularPath = true;
    // const BsdfContext ctx{};
    // for (;; depth++) {
    //   if (ray.MaxT <= ray.MinT) {
    //     break;
    //   }
    //   SurfaceInteraction si{};
    //   bool isHit = scene.RayIntersect(ray, si);
    //   MediumInteraction mi{};
    //   if (rayMedium != nullptr) {
    //     Ray mediumRay{ray.O, ray.D, ray.MinT, isHit ? si.T : std::numeric_limits<Float>::max()};
    //     Spectrum tr;
    //     std::tie(mi, tr) = rayMedium->Sample(mediumRay, *sampler);
    //     throughput = Spectrum(throughput.cwiseProduct(tr));
    //   }
    //   if (throughput.IsBlack()) {
    //     break;
    //   }
    //   if (mi.IsValid()) {
    //     Spectrum lo = DirectIlluminationMedium(scene, *sampler, mi, *rayMedium);
    //     result += Spectrum(throughput.cwiseProduct(lo));
    //     const PhaseFunction& phase = mi.Medium->GetPhaseFunction();
    //     auto [wo, phasePdf] = phase.Sample(mi, sampler->Next2D());
    //     ray = mi.SpawnRay(wo);
    //     isSpecularPath = false;
    //   } else {
    //     if (depth == 0 || isSpecularPath) {
    //       std::optional<Light*> light = scene.GetLight(si);
    //       if (light.has_value()) {
    //         Spectrum le = (*light)->Eval(si);
    //         result += Spectrum(throughput.cwiseProduct(le));
    //       }
    //     }
    //     if (!isHit) {
    //       break;
    //     }
    //     if (_maxDepth >= 0 && depth + 1 >= _maxDepth) {
    //       break;
    //     }
    //     if (!si.Shape->HasBsdf()) {
    //       rayMedium = si.GetMedium(ray);
    //       ray = si.SpawnRay(ray.D);
    //       continue;
    //     }
    //     Bsdf* bsdf = si.BSDF(ray);
    //     Spectrum lo = DirectIlluminationSurface(scene, ctx, *sampler, si, rayMedium);
    //     result += Spectrum(throughput.cwiseProduct(lo));
    //     auto [bsr, fs] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
    //     if (bsr.Pdf > 0) {
    //       throughput = Spectrum(throughput.cwiseProduct(fs) / bsr.Pdf);
    //       eta *= bsr.Eta;
    //     } else {
    //       break;
    //     }
    //     ray = si.SpawnRay(si.ToWorld(bsr.Wo));
    //     rayMedium = si.GetMedium(ray);
    //     isSpecularPath = bsr.HasType(BsdfType::Delta);
    //   }
    //   if (depth >= _rrDepth) {
    //     Float maxThroughput = throughput.MaxComponent();
    //     Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
    //     if (sampler->Next1D() > rr) {
    //       break;
    //     }
    //     throughput *= Math::Rcp(rr);
    //   }
    // }
    // return result;

    RayDifferential ray = ray_;
    const Medium* rayMedium = scene.GetGlobalMedium();
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    bool isSpecularPath = true;
    Interaction prevInter{};
    Float prevPdf = 1;
    BsdfContext ctx{};
    for (;; depth++) {
      MediumInteraction mi{};
      SurfaceInteraction si{};
      bool isHit = scene.RayIntersect(ray, si);
      if (rayMedium != nullptr) {
        Ray mediumRay{ray.O, ray.D, ray.MinT, isHit ? si.T : std::numeric_limits<Float>::max()};
        Spectrum tr;
        std::tie(mi, tr) = rayMedium->Sample(mediumRay, *sampler);
        throughput = Spectrum(throughput.cwiseProduct(tr));
      }
      if (mi.IsValid()) {
        //在介质上采样成功
        const PhaseFunction& phase = mi.Medium->GetPhaseFunction();
        auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
        if (dsr.Pdf > 0) {
          Vector3 wo = dsr.Dir;
          Float p = phase.Eval(mi, wo);
          Float phasePdf = p;
          Spectrum transmittance = scene.Transmittance(mi, dsr.P, rayMedium, *sampler);
          if (!transmittance.IsBlack() && !li.IsBlack()) {
            Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, phasePdf);
            Spectrum le(throughput.cwiseProduct(transmittance).cwiseProduct(li) * p * weight / dsr.Pdf);
            result += le;
          }
        }
        auto [dir, phaseVal] = phase.Sample(mi, sampler->Next2D());
        if (phaseVal <= 0) {
          break;
        }
        ray = mi.SpawnRay(dir);
        prevInter = mi;
        prevPdf = phaseVal;
        isSpecularPath = false;
      } else {
        std::optional<Light*> light = scene.GetLight(si);
        if (light) {
          Float weight;
          if (isSpecularPath) {
            weight = 1;
          } else {
            DirectionSampleResult dsr = si.ToDsr(prevInter);
            Float lightPdf = scene.PdfLightDirection(*light, prevInter, dsr);
            weight = MisWeight(prevPdf, lightPdf);
          }
          Spectrum li = (*light)->Eval(si);
          Spectrum lo(throughput.cwiseProduct(li) * weight);
          result += lo;
        }
        if (!isHit || si.Shape->IsLight()) {
          break;
        }
        if (_maxDepth >= 0 && depth + 1 >= _maxDepth) {
          break;
        }
        if (!si.Shape->HasBsdf()) {
          rayMedium = si.GetMedium(ray);
          ray = si.SpawnRay(ray.D);
          continue;
        }
        Bsdf* bsdf = si.BSDF(ray);
        if (bsdf->HasAnyTypeExceptDelta()) {
          auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
          if (dsr.Pdf > 0) {
            Vector3 wo = si.ToLocal(dsr.Dir);
            Spectrum fs = bsdf->Eval(ctx, si, wo);
            Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
            Spectrum transmittance = scene.Transmittance(si, dsr.P, rayMedium, *sampler);
            if (!transmittance.IsBlack() && !li.IsBlack()) {
              Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, bsdfPdf);
              Spectrum le(throughput.cwiseProduct(fs).cwiseProduct(li).cwiseProduct(transmittance) * weight / dsr.Pdf);
              result += le;
            }
          }
        }
        {
          auto [bsr, fs] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
          if (bsr.Pdf <= 0) {
            break;
          } else {
            throughput = Spectrum(throughput.cwiseProduct(fs) / bsr.Pdf);
            eta *= bsr.Eta;
          }
          ray = si.SpawnRay(si.ToWorld(bsr.Wo));
          rayMedium = si.GetMedium(ray);
          prevInter = si;
          prevPdf = bsr.Pdf;
          isSpecularPath = bsr.HasType(BsdfType::Delta);
        }
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
  }

  Float MisWeight(Float a, Float b) const {
    a *= a;
    b *= b;
    Float w = a / (a + b);
    return std::isfinite(w) ? w : 0;
  }

  Spectrum DirectIlluminationMedium(
      const Scene& scene,
      Sampler& sampler,
      const MediumInteraction& mi,
      const Medium& medium) const {
    Spectrum result(0);
    const PhaseFunction& phase = mi.Medium->GetPhaseFunction();
    {
      auto [l, dsr, li] = scene.SampleLightDirection(mi, sampler.Next1D(), sampler.Next2D());
      if (dsr.Pdf > 0 && !li.IsBlack()) {
        Float ps = phase.Eval(mi, dsr.Dir);
        Float phasePdf = ps;
        if (ps > 0) {
          Spectrum tr = scene.Transmittance(mi, dsr.P, &medium, sampler);
          if (!tr.IsBlack()) {
            Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, phasePdf);
            result += Spectrum(li.cwiseProduct(tr) * ps * weight / dsr.Pdf);
          }
        }
      }
    }
    {
      auto [wo, ps] = phase.Sample(mi, sampler.Next2D());
      Float phasePdf = ps;
      if (phasePdf > 0) {
        Ray sampledRay = mi.SpawnRay(wo);
        auto [_, tr, si] = scene.RayIntersectWithTransmittance(sampledRay, &medium, sampler);
        std::optional<Light*> light = scene.GetLight(si);
        if (light.has_value()) {
          Spectrum li = (*light)->Eval(si);
          DirectionSampleResult dsr = si.ToDsr(mi);
          Float lightPdf = scene.PdfLightDirection(*light, mi, dsr);
          Float weight = MisWeight(phasePdf, lightPdf);
          result += Spectrum(li.cwiseProduct(tr) * ps * weight / phasePdf);
        }
      }
    }
    return result;
  }

  Spectrum DirectIlluminationSurface(
      const Scene& scene,
      const BsdfContext& ctx,
      Sampler& sampler,
      const SurfaceInteraction& si,
      const Medium* medium) const {
    Spectrum result(0);
    const Bsdf* bsdf = si.Shape->GetBsdf();
    auto [l, dsr, li] = scene.SampleLightDirection(si, sampler.Next1D(), sampler.Next2D());
    if (bsdf->HasAnyTypeExceptDelta()) {
      if (dsr.Pdf > 0 && !li.IsBlack()) {
        Vector3 wo = si.ToLocal(dsr.Dir);
        Spectrum fs = bsdf->Eval(ctx, si, wo);
        Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
        if (bsdfPdf > 0) {
          Spectrum tr = scene.Transmittance(si, dsr.P, medium, sampler);
          if (!tr.IsBlack()) {
            Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, bsdfPdf);
            result += Spectrum(li.cwiseProduct(tr).cwiseProduct(fs) * weight / dsr.Pdf);
          }
        }
      }
    }
    if (!dsr.IsDelta) {
      auto [bsr, fs] = bsdf->Sample(ctx, si, sampler.Next1D(), sampler.Next2D());
      if (bsr.Pdf > 0 && !fs.IsBlack()) {
        Ray sampledRay = si.SpawnRay(si.ToWorld(bsr.Wo));
        auto [isHit, tr, hit] = scene.RayIntersectWithTransmittance(sampledRay, medium, sampler);
        std::optional<Light*> hitLight = scene.GetLight(hit);
        if (hitLight.has_value() && (*hitLight) == l) {
          Float weight;
          if (bsr.HasType(BsdfType::Delta)) {
            weight = 1;
          } else {
            Float lightPdf = l->PdfDirection(si, hit.ToDsr(si));
            weight = MisWeight(bsr.Pdf, lightPdf);
          }
          result += Spectrum(li.cwiseProduct(tr).cwiseProduct(fs) * weight / bsr.Pdf);
        }
      }
    }
    // {
    //   auto [bsr, fs] = bsdf->Sample(ctx, si, sampler.Next1D(), sampler.Next2D());
    //   if (bsr.Pdf > 0 && !fs.IsBlack()) {
    //     Ray sampledRay = si.SpawnRay(si.ToWorld(bsr.Wo));
    //     auto [isHit, tr, hit] = scene.RayIntersectWithTransmittance(sampledRay, medium, sampler);
    //     std::optional<Light*> hitLight = scene.GetLight(hit);
    //     if (hitLight.has_value()) {
    //       Float weight;
    //       if (bsr.HasType(BsdfType::Delta)) {
    //         weight = 1;
    //       } else {
    //         Float lightPdf = scene.PdfLightDirection(*hitLight, si, hit.ToDsr(si));
    //         weight = MisWeight(bsr.Pdf, lightPdf);
    //       }
    //       Spectrum li = (*hitLight)->Eval(hit);
    //       result += Spectrum(li.cwiseProduct(tr).cwiseProduct(fs) * weight / bsr.Pdf);
    //     }
    //   }
    // }
    return result;
  }

 private:
  Int32 _maxDepth;
  Int32 _rrDepth;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(VolPath, "vol_path");
