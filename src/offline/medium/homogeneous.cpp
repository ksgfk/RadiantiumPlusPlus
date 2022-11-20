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
    _isHomogeneous = true;
    _hasSpectralExtinction = true;
  }

  std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const override {
    return {true, 0.f, std::numeric_limits<Float>::infinity()};
  }

  Spectrum GetMajorant(const MediumInteraction& mi) const override {
    return eval_sigmat(mi);
  }

  std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const override {
    Spectrum sigmat = eval_sigmat(mi);
    Spectrum sigmas(sigmat.cwiseProduct(_albedo));
    Spectrum sigman(0);
    return {sigmas, sigman, sigmat};
  }

  Spectrum eval_sigmat(const MediumInteraction& mi) const {
    auto sigmat = _sigmaT * _scale;
    return Spectrum(sigmat);
  }

 private:
  Spectrum _sigmaT;
  Spectrum _albedo;
  Float _scale;
};

}  // namespace Rad

RAD_FACTORY_MEDIUM_DECLARATION(HomogeneousMedium, "homogeneous");
