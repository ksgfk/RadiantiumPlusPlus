#include <rad/offline/render/medium.h>

#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>

namespace Rad {

Medium::Medium(BuildContext* ctx, const ConfigNode& cfg) {
  ConfigNode phaseNode;
  if (cfg.TryRead("phase", phaseNode)) {
    std::string type = phaseNode.Read<std::string>("type");
    PhaseFunctionFactory* factory = ctx->GetFactoryManager().GetFactory<PhaseFunctionFactory>(type);
    _phaseFunction = factory->Create(ctx, phaseNode);
  } else {
    throw RadArgumentException("medium must have phase function");
  }
}

MediumInteraction Medium::SampleInteraction(const Ray& ray, Float sample, UInt32 channel) const {
  MediumInteraction mei{};
  mei.Wi = -ray.D;
  mei.Shading = Frame(mei.Wi);
  auto [aabbIts, mint, maxt] = IntersectAABB(ray);
  aabbIts &= (std::isfinite(mint) || std::isfinite(maxt));
  bool active = aabbIts;
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
  if (t <= 0) {
    return std::make_pair(Spectrum(1), Spectrum(1));
  }
  Spectrum tr = ExpSpectrum(Spectrum(mi.CombinedExtinction * -t));
  Spectrum pdf = si.T < mi.T ? tr : Spectrum(tr.cwiseProduct(mi.CombinedExtinction));
  // if (tr.HasNaN() || tr.HasInfinity() || pdf.HasNaN() || pdf.HasInfinity() || tr.isZero(0.0001f)) {
  //   Logger::Get()->warn("emmmmm {} {} {}", t, tr, pdf);
  // }
  if (tr.cwiseEqual(pdf).all()) {
    return {Spectrum(1), Spectrum(1)};
  }
  return {tr, pdf};
}

}  // namespace Rad
