#include <rad/offline/render/medium.h>

#include <rad/offline/build.h>

namespace Rad {

Medium::Medium(BuildContext* ctx, const ConfigNode& cfg) {
  ConfigNode phaseNode;
  if (cfg.TryRead("phase", phaseNode)) {
    std::string type = phaseNode.Read<std::string>("type");
    PhaseFunctionFactory* factory = ctx->GetFactory<PhaseFunctionFactory>(type);
    _phaseFunction = factory->Create(ctx, phaseNode);
  } else {
    throw RadArgumentException("medium must have phase function");
  }
}

MediumInteraction Medium::SampleInteraction(const Ray& ray, Float sample, UInt32 channel) const {
  MediumInteraction mei{};
  mei.Wi = -ray.D;
  mei.Shading = Frame(mei.Wi);
  auto [aabb_its, mint, maxt] = IntersectAABB(ray);
  aabb_its &= (std::isfinite(mint) || std::isfinite(maxt));
  bool active = aabb_its;
  if (!active) {
    mint = 0;
    maxt = std::numeric_limits<Float>::infinity();
  }
  mint = std::max(Float(0), mint);
  maxt = std::min(ray.MaxT, maxt);
  auto combinedExtinction = GetMajorant(mei);
  Float m = combinedExtinction[channel];
  Float sampledT = mint + (-std::log(1 - sample) / m);
  bool validMi = active && (sampledT <= maxt);
  mei.T = validMi ? sampledT : std::numeric_limits<Float>::infinity();
  mei.P = ray(sampledT);
  mei.Medium = (Medium*)this;
  mei.MinT = mint;
  std::tie(mei.SigmaS, mei.SigmaN, mei.SigmaT) = GetScatteringCoefficients(mei);
  mei.CombinedExtinction = combinedExtinction;
  return mei;
}

std::pair<Spectrum, Spectrum> Medium::EvalTrAndPdf(const MediumInteraction& mi, const SurfaceInteraction& si) const {
  Float t = std::min(mi.T, si.T) - mi.MinT;
  Spectrum tr = ExpSpectrum(Spectrum(mi.CombinedExtinction * -t));
  Spectrum pdf = si.T < mi.T ? tr : Spectrum(tr.cwiseProduct(mi.CombinedExtinction));
  return {tr, pdf};
}

}  // namespace Rad
