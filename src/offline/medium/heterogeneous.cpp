#include <rad/offline/render/medium.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <embree3/rtcore.h>

namespace Rad {

class HeterogeneousMedium final : public Medium {
 public:
  HeterogeneousMedium(BuildContext* ctx, const ConfigNode& cfg) : Medium(ctx, cfg) {
    _sigmaT = cfg.ReadVolume(*ctx, "sigma_t", Spectrum(Float(1)));
    _albedo = cfg.ReadVolume(*ctx, "albedo", Spectrum(Float(0.75)));
    _scale = cfg.ReadOrDefault("scale", Float(1));
    _maxDensity = _sigmaT->GetMaxValue() * _scale;
    _isHomogeneous = false;
    _hasSpectralExtinction = true;
  }

  std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const override {
    return BoundingBox3RayIntersect(_sigmaT->GetBoundingBox(), ray);
  }

  Spectrum GetMajorant(const MediumInteraction& mi) const override {
    return _maxDensity;
  }

  std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const override {
    Spectrum sigmat = EvalSigmaT(mi);
    Spectrum sigmas(sigmat.cwiseProduct(_albedo->Eval(mi)));
    Spectrum sigman(Spectrum::Constant(_maxDensity) - sigmat);
    return {sigmas, sigman, sigmat};
  }

  Spectrum EvalSigmaT(const MediumInteraction& mi) const {
    Spectrum e = _sigmaT->Eval(mi);
    Spectrum sigmat(e * _scale);
    return sigmat;
  }

 private:
  static std::tuple<bool, Float, Float> BoundingBox3RayIntersect(
      const BoundingBox3& box,
      const Ray& ray) {
    auto active = !ray.D.isZero(0) || box.contains(ray.O);
    auto d_rcp = ray.D.cwiseInverse();
    auto t1 = (box.min() - ray.O).cwiseProduct(d_rcp);
    auto t2 = (box.max() - ray.O).cwiseProduct(d_rcp);
    auto t1p = t1.cwiseMin(t2);
    auto t2p = t1.cwiseMax(t2);
    Float mint = t1p.maxCoeff();
    Float maxt = t2p.minCoeff();
    active = active && maxt >= mint;
    return std::make_tuple(active, mint, maxt);
  }

  Unique<Volume> _sigmaT;
  Unique<Volume> _albedo;
  Float _scale;
  Float _maxDensity;
};

}  // namespace Rad

RAD_FACTORY_MEDIUM_DECLARATION(HeterogeneousMedium, "heterogeneous");
