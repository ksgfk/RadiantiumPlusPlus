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

MediumInteraction Medium::SampleInteraction(const Ray& ray, Float xi, UInt32 channel) const {
  MediumInteraction mei{};
  mei.Wi = -ray.D;
  mei.Shading = Frame(mei.Wi);
  auto [isHit, mint, maxt] = IntersectAABB(ray);
  isHit &= (std::isfinite(mint) || std::isfinite(maxt));
  if (!isHit) {
    mint = 0;
    maxt = std::numeric_limits<Float>::infinity();
  }
  mint = std::max(Float(0), mint);
  maxt = std::min(ray.MaxT, maxt);
  Spectrum extinction = GetMajorant(mei);
  Float m = extinction[0];
  if (channel == 1) {
    m = extinction[1];
  }
  if (channel == 2) {
    m = extinction[2];
  }
  Float sampledT = mint + (-std::log(1 - xi) / m);
  if (isHit && sampledT <= maxt) {
    mei.T = sampledT;
    std::tie(mei.SigmaS, mei.SigmaN, mei.SigmaT) = GetScattingCoeff(mei);
  } else {
    mei.T = std::numeric_limits<Float>::infinity();
  }
  mei.P = ray(sampledT);
  mei.Medium = (Medium*)this;
  mei.MinT = mint;
  mei.Extinction = extinction;
  return mei;
}

std::pair<Spectrum, Spectrum> Medium::EvalTrAndPdf(const MediumInteraction& mi, const SurfaceInteraction& si) const {
  Float t = std::min(std::numeric_limits<Float>::max(), std::min(mi.T, si.T)) - mi.MinT;
  auto tmp = (mi.Extinction * (-t));
  Spectrum tr(std::exp(tmp.x()), std::exp(tmp.y()), std::exp(tmp.z()));
  Spectrum pdf = si.T < mi.T ? tr : Spectrum(tr.cwiseProduct(mi.Extinction));
  return std::make_pair(tr, pdf);
}

}  // namespace Rad
