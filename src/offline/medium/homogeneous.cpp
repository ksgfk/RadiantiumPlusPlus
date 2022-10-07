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

  std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const override {
    return std::make_tuple(true, Float(0), std::numeric_limits<Float>::infinity());
  }

  Spectrum EvalSigmaT(const MediumInteraction& mi) const {
    Spectrum sigmat(_sigmaT * _scale);
    return sigmat;
  }

  Spectrum GetMajorant(const MediumInteraction& mi) const override {
    return EvalSigmaT(mi);
  }

  std::tuple<Spectrum, Spectrum, Spectrum> GetScattingCoeff(const MediumInteraction& mi) const override {
    Spectrum sigmat = EvalSigmaT(mi);
    Spectrum sigmas(sigmat.cwiseProduct(_albedo));
    Spectrum sigman(Float(0));
    return std::make_tuple(sigmas, sigman, sigmat);
  }

 private:
  Spectrum _sigmaT;
  Spectrum _albedo;
  Float _scale;
};

}  // namespace Rad

RAD_FACTORY_MEDIUM_DECLARATION(HomogeneousMedium, "homogeneous");
