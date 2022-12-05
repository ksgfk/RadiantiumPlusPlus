#include <rad/offline/render/medium.h>

#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/volume.h>

namespace Rad {

/**
 * @brief 均匀介质
 */
class HomogeneousMedium final : public Medium {
 public:
  HomogeneousMedium(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) : Medium(ctx, cfg) {
    Color24f sigmaT = cfg.ReadOrDefault("sigma_t", Color24f(1, 1, 1));
    _sigmaT = Color24fToSpectrum(sigmaT);
    _albedo = ConfigNodeReadVolume(ctx, cfg, "albedo", Spectrum(Float(0.75)));
    _scale = cfg.ReadOrDefault("scale", Float(1));
    _isHomogeneous = true;
    _hasSpectralExtinction = true;
  }

  std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const override {
    return {true, 0.f, std::numeric_limits<Float>::infinity()};
  }

  Spectrum GetMajorant(const MediumInteraction& mi) const override {
    return EvalSigmaT(mi);
  }

  std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const override {
    Spectrum sigmat = EvalSigmaT(mi);
    Spectrum sigmas(sigmat.cwiseProduct(_albedo->Eval(mi)));
    Spectrum sigman(0);
    return {sigmas, sigman, sigmat};
  }

  Spectrum EvalSigmaT(const MediumInteraction& mi) const {
    auto sigmat = _sigmaT * _scale;
    return Spectrum(sigmat);
  }

 private:
  Spectrum _sigmaT;
  Unique<Volume> _albedo;
  Float _scale;
};

class HomogeneousMediumFactory : public MediumFactory {
 public:
  HomogeneousMediumFactory() : MediumFactory("homogeneous") {}
  virtual ~HomogeneousMediumFactory() noexcept = default;
  Unique<Medium> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<HomogeneousMedium>(ctx, toWorld, cfg);
  }
};

Unique<MediumFactory> _FactoryCreateHomogeneousMediumFunc_() {
  return std::make_unique<HomogeneousMediumFactory>();
}

}  // namespace Rad
