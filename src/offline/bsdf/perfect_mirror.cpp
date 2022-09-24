#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/render/fresnel.h>

namespace Rad {

/**
 * @brief 这个BRDF描述了一种表面完美光滑的导体材质, 因此它的分布是一个delta函数
 * 也就是说, 只有一个特定方向才能评估出函数值
 * 将这种BRDF带入渲染方程, 可以直接写成 Lo(wo) = f(wr) * Li(wr), wr是镜面反射方向
 */
class PerfectMirror final : public Bsdf {
 public:
  PerfectMirror(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Delta | BsdfType::Reflection;
    _reflectance = cfg.ReadTexture(*ctx, "reflectance", Color(Float(1)));
    _eta = cfg.ReadTexture(*ctx, "eta", Color(Float(1)));
    _k = cfg.ReadTexture(*ctx, "k", Color(Float(1)));
  }
  ~PerfectMirror() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    if (!context.IsEnable(BsdfType::Delta, BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    BsdfSampleResult bsr{};
    bsr.Wo = Fresnel::Reflect(si.Wi);
    bsr.Eta = 1;
    bsr.TypeMask = _flags;
    bsr.Pdf = 1;
    Spectrum r = _reflectance->Eval(si);
    Spectrum F = Fresnel::Conductor(Frame::CosTheta(si.Wi), _eta->Eval(si), _k->Eval(si));
    auto result = r.cwiseProduct(F);
    return {bsr, Spectrum(result)};
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
  Share<Texture<Color>> _reflectance;
  Share<Texture<Color>> _eta;
  Share<Texture<Color>> _k;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(PerfectMirror, "perfect_mirror");
