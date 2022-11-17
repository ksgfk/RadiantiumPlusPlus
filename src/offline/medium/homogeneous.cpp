#include <rad/offline/render/medium.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class HomogeneousMedium final : public Medium {
 public:
  HomogeneousMedium(BuildContext* ctx, const ConfigNode& cfg) : Medium(ctx, cfg) {
    Vector3 sigmaT = cfg.ReadOrDefault("sigma_t", Vector3(1, 1, 1));
    Vector3 albedo = cfg.ReadOrDefault("albedo", Vector3(Vector3::Constant(Float(0.75))));
    _sigmaT = Spectrum(sigmaT);
    _albedo = Spectrum(albedo);
    _scale = cfg.ReadOrDefault("scale", Float(1));
  }

  Spectrum Tr(const Ray& ray, Sampler& sampler) const override {
    Spectrum sigmaT(_sigmaT * _scale);
    Spectrum tr = ExpSpectrum(Spectrum(-sigmaT * (ray.MaxT - ray.MinT)));
    return tr;
  }

  std::pair<MediumInteraction, Spectrum> Sample(const Ray& ray, Sampler& sampler) const override {
    Int32 channel = std::min((Int32)(sampler.Next1D() * Spectrum::ComponentCount), (Int32)Spectrum::ComponentCount - 1);
    Spectrum sigmaT(_sigmaT * _scale);
    Float samplingDensity = sigmaT[channel];
    Float sampledDist = -std::log(1 - sampler.Next1D()) / samplingDensity;
    Float rayDist = ray.MaxT - ray.MinT;
    MediumInteraction mi{};
    Spectrum result;
    if (sampledDist < rayDist) {
      Float t = ray.MinT + sampledDist;
      Spectrum tr = ExpSpectrum(Spectrum(-sigmaT * t));
      Float pdf = sigmaT.cwiseProduct(tr).sum() * (Float(1) / Spectrum::ComponentCount);
      mi.T = t;
      mi.P = ray(t);
      mi.N = ray.D;
      mi.Wi = -ray.D;
      mi.Shading = Frame(mi.N);
      mi.Medium = (Medium*)this;
      result = Spectrum(tr.cwiseProduct(sigmaT.cwiseProduct(_albedo)) / pdf);
    } else {
      Float t = rayDist;
      Spectrum tr = ExpSpectrum(Spectrum(-sigmaT * t));
      Float pdf = tr.sum() * (Float(1) / Spectrum::ComponentCount);
      result = Spectrum(tr / pdf);
    }
    return std::make_pair(mi, result);
  }

 private:
  Spectrum _sigmaT;
  Spectrum _albedo;
  Float _scale;
};

}  // namespace Rad

RAD_FACTORY_MEDIUM_DECLARATION(HomogeneousMedium, "homogeneous");
