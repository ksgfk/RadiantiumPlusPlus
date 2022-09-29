#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>

namespace Rad {

/**
 * @brief 最普通的漫反射材质, 也就是Lambertian, 这种材质会将接收到的所有辐射均匀散射出去
 * 需要注意的是, 这是一种单面的材质
 *
 * 为什么计算时还要将反射率除以PI?
 * 基于物理的渲染中很重要的一点是保证能量守恒
 * 什么是能量守恒?
 * 着色点向所有方向出射的radiance总和必须小于等于收到的irradiance
 * 计算出射的所有radiance很简单
 * Eo = ∫ Lo cosθ dω, 积分域是所有可能的出射方向, Lo是出射方向的radiance, 加上cosine项是因为Lo定义在单位立体角上
 * 回顾一下BRDF定义
 * f() = Lo / Ei, Lo表示出射方向的radiance, Ei表示收到的irradiance
 * Lambertian的BRDF结果永远是个常数R, 因此 Lo = R * Ei
 * 将Lo带入积分, 结合能量守恒可以得到一个不等式
 * ∫ R * Ei * cosθ dω <= Ei
 * R与Ei都是常数, 由于BRDF积分域在上半球上, 所以需要将微元从立体角转化到极坐标系
 * 如何在半球上积分: https://chengkehan.github.io/HemisphericalCoordinates.html
 * 众所周知, dω = sinθdθdφ
 * 因此, 积分变成了
 * R\int_{0}^{2\pi}\int_{0}^{\pi}\mathrm{cos}\theta\mathrm{sin}\theta\mathrm{d}\theta\mathrm{d}\phi\leq 1
 * 不等式左边是个定积分, 可以直接解出结果
 * R\pi\leq 1
 * 因此, R\leq\frac{1}{\pi}
 * 结果显示, Lambertian BRDF的常数R范围是[0,\frac{1}{\pi}], 只有在这个区间才能保证能量守恒
 * 所以, 才需要将反射率除以PI
 */
class Diffuse final : public Bsdf {
 public:
  Diffuse(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Diffuse | BsdfType::Reflection;
    _reflectance = cfg.ReadTexture(*ctx, "reflectance", Color(Float(0.5)));
  }
  ~Diffuse() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    BsdfSampleResult bsr{};
    bsr.Wo = Warp::SquareToCosineHemisphere(dirXi);
    bsr.Pdf = Warp::SquareToCosineHemispherePdf(bsr.Wo);
    bsr.Eta = Float(1);
    bsr.TypeMask = _flags;
    Spectrum reflectance = Spectrum(_reflectance->Eval(si));
    Float cosThetaO = Frame::CosTheta(bsr.Wo);
    auto f = reflectance * (1 / Math::PI) * cosThetaO;
    return std::make_pair(bsr, Spectrum(f));
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return Spectrum(0);
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return Spectrum(0);
    }
    Spectrum reflectance = Spectrum(_reflectance->Eval(si));
    auto f = reflectance * (1 / Math::PI) * cosThetaO;
    return Spectrum(f);
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    if (!context.IsEnable(BsdfType::Diffuse, BsdfType::Reflection)) {
      return 0;
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    Float cosThetaO = Frame::CosTheta(wo);
    if (cosThetaI <= 0 || cosThetaO <= 0) {
      return 0;
    }
    Float pdf = Warp::SquareToCosineHemispherePdf(wo);
    return pdf;
  }

 private:
  Unique<TextureRGB> _reflectance;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(Diffuse, "diffuse");
