#include <rad/offline/render/bsdf.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/fresnel.h>
#include <rad/offline/math_ext.h>

namespace Rad {

/**
 * @brief 这个BSDF模拟的是两种不同折射率的电介质材料的表面, 内外的折射率可以分别指定
 * 由菲涅尔决定光线在表面是透射还是反射
 * 表面是完美光滑的, 因此它的分布是一个delta函数
 */
class PerfectGlass final : public Bsdf {
 public:
  PerfectGlass(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Delta | BsdfType::Reflection | BsdfType::Transmission;
    _reflectance = ConfigNodeReadTexture(ctx, cfg, "reflectance", Color24f(1));
    _transmittance = ConfigNodeReadTexture(ctx, cfg, "transmittance", Color24f(1));
    Float intIor = cfg.ReadOrDefault("int_ior", 1.5046f);    //内部折射率
    Float extIor = cfg.ReadOrDefault("ext_ior", 1.000277f);  //外部折射率
    _eta = intIor / extIor;
  }
  ~PerfectGlass() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    if (!context.IsEnable(BsdfType::Delta)) {
      return {{}, Spectrum(0)};
    }
    bool hasReflection = context.IsEnable(BsdfType::Reflection);
    bool hasTransmission = context.IsEnable(BsdfType::Transmission);
    Float cosThetaI = Frame::CosTheta(si.Wi);
    auto [ri, cosThetaT, etaIT, etaTI] = Fresnel::Dielectric(cosThetaI, _eta);
    Float ti = 1 - ri;
    bool selectReflection = lobeXi <= ri;
    BsdfSampleResult bsr{};
    Spectrum f;
    if (selectReflection) {
      if (!hasReflection) {
        return {{}, Spectrum(0)};
      }
      bsr.Wo = Fresnel::Reflect(si.Wi);
      bsr.Eta = 1;
      bsr.Pdf = ri;
      bsr.TypeMask = BsdfType::Delta | BsdfType::Reflection;
      Spectrum r(_reflectance->Eval(si));
      f = Spectrum(r * ri);
    } else {
      if (!hasTransmission) {
        return {{}, Spectrum(0)};
      }
      bsr.Wo = Fresnel::Refract(si.Wi, cosThetaT, etaTI);
      bsr.Eta = etaIT;
      bsr.Pdf = ti;
      bsr.TypeMask = BsdfType::Delta | BsdfType::Transmission;
      Spectrum t(_transmittance->Eval(si));
      f = Spectrum(t * ti);
      Float factor = (context.Mode == TransportMode::Radiance) ? etaTI : Float(1);
      f *= Math::Sqr(factor);
    }
    return {bsr, f};
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    return Spectrum(0);
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    return 0;
  }

 private:
  Unique<TextureRGB> _reflectance;
  Unique<TextureRGB> _transmittance;
  Float _eta;
};

class PerfectGlassFactory final : public BsdfFactory {
 public:
  PerfectGlassFactory() : BsdfFactory("perfect_glass") {}
  ~PerfectGlassFactory() noexcept override = default;
  Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<PerfectGlass>(ctx, cfg);
  }
};

Unique<BsdfFactory> _FactoryCreatePerfectGlassFunc_() {
  return std::make_unique<PerfectGlassFactory>();
}

}  // namespace Rad
