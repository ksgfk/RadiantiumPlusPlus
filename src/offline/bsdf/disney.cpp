#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/render/microfacet.h>
#include <rad/offline/render/fresnel.h>

using namespace Rad::Math;

namespace Rad {

/**
 * @brief Disney principled BSDF
 * 这是disney在 https://www.disneyanimation.com/publications/physically-based-shading-at-disney/
 * 和 https://blog.selfshadow.com/publications/s2015-shading-course/#course_content
 * 提出的一种, 非常复杂的BSDF, 它拥有非常多的输入参数, 其中大多数的有效范围都是0到1
 * 这些参数对于艺术家很友好, 它们与真实的物理参数不是一一对应的
 * 这种BSDF可以模拟非常多的材质, 虽然目前这里的实现不支持次表面散射
 * 详细解析: 
 * https://zhuanlan.zhihu.com/p/60977923
 * https://zhuanlan.zhihu.com/p/57771965
 * https://zhuanlan.zhihu.com/p/407007915
 */
class Disney final : public Bsdf {
 public:
  /**
   * @brief Generalized Trowbridge-Reitz
   * 它是GGX法线分布函数的广义形式, 它的公式是:
   * D(m)=\frac{c}{(1+(n\cdots m)^{2}(\alpha^{2}-1))^{\gamma}}
   * 其中, \gamma 用来控制尾部形状, \gamma=2时是原版GGX
   * 这里取\gamma=1的形式
   */
  struct GTR1 {
    //采样没有使用 visible normal sampling, 因为本身GTR1只用于clearcoat, 具有的能量较小
    Vector3 Sample(const Vector2& xi) const {
      auto [sinPhi, cosPhi] = SinCos((Float(2) * PI) * xi.x());
      Float alpha2 = Sqr(alpha);
      Float cosTheta2 = (Float(1) - std::pow(alpha2, Float(1) - xi.y())) / (Float(1) - alpha2);
      Float sinTheta = std::sqrt(std::max(Float(0), Float(1) - cosTheta2));
      Float cosTheta = std::sqrt(std::max(Float(0), cosTheta2));
      return Vector3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
    }

    Float Eval(const Vector3& m) const {
      Float cosTheta = Frame::CosTheta(m);
      Float cosTheta2 = Sqr(cosTheta), alpha2 = Sqr(alpha);
      Float result = (alpha2 - Float(1)) / (PI * std::log(alpha2) * (Float(1) + (alpha2 - Float(1)) * cosTheta2));
      return (result * cosTheta > Float(1e-20f) ? result : Float(0));
    }

    Float Pdf(const Vector3& m) const {
      return (m.z() < Float(0) ? Float(0) : Frame::CosTheta(m) * Eval(m));
    }

    Float alpha;
  };

  Disney(BuildContext* ctx, const ConfigNode& cfg) {
    _baseColor = cfg.ReadTexture(*ctx, "base_color", Color(Float(0.5)));
    _roughness = cfg.ReadTexture(*ctx, "roughness", Float(0.5));
    _hasAnisotropic = ConfigHasValue("anisotropic", cfg);
    _anisotropic = cfg.ReadTexture(*ctx, "anisotropic", Float(0));
    _hasSpecTrans = ConfigHasValue("spec_trans", cfg);
    _specTrans = cfg.ReadTexture(*ctx, "spec_trans", Float(0));
    _hasSheen = ConfigHasValue("sheen", cfg);
    _sheen = cfg.ReadTexture(*ctx, "sheen", Float(0.5));
    _hasSheenTint = ConfigHasValue("sheen_tint", cfg);
    _sheen_tint = cfg.ReadTexture(*ctx, "sheen_tint", Float(0));
    _hasFlatness = ConfigHasValue("flatness", cfg);
    _flatness = cfg.ReadTexture(*ctx, "flatness", Float(0));
    _hasSpecTint = ConfigHasValue("spec_tint", cfg);
    _specTint = cfg.ReadTexture(*ctx, "spec_tint", Float(0));
    _hasMetallic = ConfigHasValue("metallic", cfg);
    _metallic = cfg.ReadTexture(*ctx, "metallic", Float(0));
    _hasClearcoat = ConfigHasValue("clearcoat", cfg);
    _clearcoat = cfg.ReadTexture(*ctx, "clearcoat", Float(0));
    _clearcoatGloss = cfg.ReadTexture(*ctx, "clearcoat_gloss", Float(0.5));
    _specularSample = cfg.ReadOrDefault("main_specular_sampling_rate", Float(1));
    _clearcoatSample = cfg.ReadOrDefault("clearcoat_sampling_rate", Float(1));
    _diffuseReflectSample = cfg.ReadOrDefault("diffuse_reflectance_sampling_rate", Float(1));

    if (cfg.HasNode("eta") && cfg.HasNode("specular")) {
      throw RadArgumentException("eta cannot set with specular at same time");
    } else if (cfg.HasNode("eta")) {
      _hasEtaSpecular = true;
      _eta = cfg.Read<Float>("eta");
      if (_hasSpecTrans && _eta == Float(1)) {
        _eta = Float(1.001);
      }
    } else {
      _hasEtaSpecular = false;
      _specular = cfg.ReadOrDefault("specular", Float(0.5));
      if (_hasSpecTrans && _specular == Float(0)) {
        _specular = Float(0.001);
      }
      _eta = Float(2) * Rcp(Float(1) - std::sqrt(Float(0.08) * _specular)) - Float(1);
    }

    _flags = BsdfType::Diffuse | BsdfType::Reflection;
    if (_hasClearcoat) {
      _flags |= (UInt32)BsdfType::Glossy;
    }
    if (_hasSpecTrans) {
      _flags |= (UInt32)BsdfType::Transmission;
    }
  }
  ~Disney() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    Float cosThetaI = Frame::CosTheta(si.Wi);
    BsdfSampleResult bsr{};
    //忽略掉完美掠射 (grazing angle)
    if (cosThetaI == 0) {
      return {bsr, Spectrum(0)};
    }
    //各种权重
    Float anisotropic = _hasAnisotropic ? _anisotropic->Eval(si) : Float(0);
    Float roughness = _roughness->Eval(si);
    Float specTrans = _hasSpecTrans ? _specTrans->Eval(si) : Float(0);
    Float metallic = _hasMetallic ? _metallic->Eval(si) : Float(0);
    Float clearcoat = _hasClearcoat ? _clearcoat->Eval(si) : Float(0);
    // BRDF与BSDF主要lobe的权重
    Float brdf = (Float(1) - metallic) * (Float(1) - specTrans);
    Float bsdf = _hasSpecTrans ? (Float(1) - metallic) * specTrans : Float(0);
    bool isFrontSide = cosThetaI > Float(0);
    //主要高光
    auto [ax, ay] = RoughnessToAlpha(anisotropic, roughness, _hasAnisotropic);
    GGX specDistr{{ax, ay}, true};
    //微表面法线
    Vector3 mSpec = std::get<0>(specDistr.Sample(MulSign(si.Wi, cosThetaI), dirXi));
    //高光反射的菲涅尔项
    auto [frSpecDielectric, cosThetaT, etaIT, etaTI] = Fresnel::Dielectric(si.Wi.dot(mSpec), _eta);
    //如果是在物体内部, 而且不存在透射, 就不再继续反射了
    if (!isFrontSide && (bsdf <= 0)) {
      return {bsr, Spectrum(0)};
    }
    //计算各种lobe的采样概率
    //在物体内部, 只采样微表面反射与透射, 其他lobe全部关闭
    Float probSpecReflect = isFrontSide
                                ? _specularSample * (Float(1) - bsdf * (Float(1) - frSpecDielectric))
                                : frSpecDielectric;
    Float probSpecTrans = _hasSpecTrans
                              ? (isFrontSide
                                     ? _specularSample * bsdf * (Float(1) - frSpecDielectric)
                                     : (Float(1) - frSpecDielectric))
                              : Float(0);
    // Clearcoat 只有高光反射1/4的能量
    Float probClearcoat = _hasClearcoat
                              ? (isFrontSide
                                     ? Float(0.25) * clearcoat * _clearcoatSample
                                     : Float(0))
                              : Float(0);
    Float probDiffuse = (isFrontSide ? brdf * _diffuseReflectSample : Float(0));
    //归一化
    Float rcpTotalProb = Rcp(probSpecReflect + probSpecTrans + probClearcoat + probDiffuse);
    probSpecTrans *= rcpTotalProb;
    probClearcoat *= rcpTotalProb;
    probDiffuse *= rcpTotalProb;
    //选择一个lobe
    Float currentProb = 0;
    bool sampleDiffuse = (lobeXi < probDiffuse);
    currentProb += probDiffuse;
    bool sampleClearcoat = _hasClearcoat && (lobeXi >= currentProb) && (lobeXi < currentProb + probClearcoat);
    currentProb += probClearcoat;
    bool sampleSpecTrans = _hasSpecTrans && (lobeXi >= currentProb) && (lobeXi < currentProb + probSpecTrans);
    currentProb += probSpecTrans;
    bool sampleSpecReflect = (lobeXi >= currentProb);
    // eta只会在透射时被更改
    bsr.Eta = Float(1);
    //采样到了高光反射
    if (sampleSpecReflect) {
      Vector3 wo = Fresnel::Reflect(si.Wi, mSpec);
      bsr.Wo = wo;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Glossy;
      //当入射与出射不在同一侧, 或者宏观与微表面法线不兼容时, 拒绝掉这次采样
      bool reflect = cosThetaI * Frame::CosTheta(wo) > Float(0);
      bool isValid = (!sampleSpecReflect || (CheckMacMicNormal(mSpec, si.Wi, wo, cosThetaI, true) && reflect));
      if (!isValid) {
        return {bsr, Spectrum(0)};
      }
    }
    //采样到了透射
    if (sampleSpecTrans) {
      Vector3 wo = Fresnel::Refract(si.Wi, mSpec, cosThetaT, etaTI);
      bsr.Wo = wo;
      bsr.TypeMask = BsdfType::Transmission | BsdfType::Glossy;
      bsr.Eta = etaIT;
      //当入射与出射不在不同侧, 或者宏观与微表面法线不兼容时, 拒绝掉这次采样
      bool refract = cosThetaI * Frame::CosTheta(wo) < Float(0);
      bool isValid = (!sampleSpecTrans || (CheckMacMicNormal(mSpec, si.Wi, wo, cosThetaI, false) && refract));
      if (!isValid) {
        return {bsr, Spectrum(0)};
      }
    }
    //采样到了能量较小的高光反射(清漆)
    if (_hasClearcoat && sampleClearcoat) {
      Float clearcoatGloss = _clearcoatGloss->Eval(si);
      //粗糙度在0.1到0.001之间, 重新插值一下
      GTR1 ccDist{Math::Lerp(Float(0.1), Float(0.001), clearcoatGloss)};
      //清漆微表面法线
      Vector3 mCC = ccDist.Sample(dirXi);
      Vector3 wo = Fresnel::Reflect(si.Wi, mCC);
      bsr.Wo = wo;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Glossy;
      bool reflect = cosThetaI * Frame::CosTheta(wo) > Float(0);
      bool isValid = (!sampleClearcoat || (CheckMacMicNormal(mCC, si.Wi, wo, cosThetaI, true) && reflect));
      if (!isValid) {
        return {bsr, Spectrum(0)};
      }
    }
    //采样到了漫反射
    if (sampleDiffuse) {
      Vector3 wo = Warp::SquareToCosineHemisphere(dirXi);
      bsr.Wo = wo;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Diffuse;
      bool reflect = cosThetaI * Frame::CosTheta(wo) > Float(0);
      bool isValid = (!sampleDiffuse || reflect);
      if (!isValid) {
        return {bsr, Spectrum(0)};
      }
    }
    bsr.Pdf = Pdf(context, si, bsr.Wo);
    if (bsr.Pdf <= 0) {
      return {bsr, Spectrum(0)};
    }
    Spectrum result = Eval(context, si, bsr.Wo);
    return std::make_pair(bsr, result);
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    Float cosThetaI = Frame::CosTheta(si.Wi);
    //忽略掉完美掠射 (grazing angle)
    if (cosThetaI == 0) {
      return Spectrum(0);
    }
    //各种权重
    Float anisotropic = _hasAnisotropic ? _anisotropic->Eval(si) : Float(0);
    Float roughness = _roughness->Eval(si);
    Float flatness = _hasFlatness ? _flatness->Eval(si) : Float(0);
    Float specTrans = _hasSpecTrans ? _specTrans->Eval(si) : Float(0);
    Float metallic = _hasMetallic ? _metallic->Eval(si) : Float(0);
    Float clearcoat = _hasClearcoat ? _clearcoat->Eval(si) : Float(0);
    Float sheen = _hasSheen ? _sheen->Eval(si) : Float(0);
    Spectrum baseColor = _baseColor->Eval(si);
    // BRDF与BSDF主要lobe的权重
    Float brdf = (Float(1) - metallic) * (Float(1) - specTrans);
    Float bsdf = (Float(1) - metallic) * specTrans;
    Float cosThetaO = Frame::CosTheta(wo);
    //判断当前方向
    bool reflect = cosThetaI * cosThetaO > Float(0);
    bool refract = cosThetaI * cosThetaO < Float(0);
    //判断入射方向
    bool isFrontSide = cosThetaI > Float(0);
    Float invEta = Rcp(_eta);
    Float etaPath = (isFrontSide ? _eta : invEta);
    Float invEtaPath = (isFrontSide ? invEta : _eta);
    //主要高光
    auto [ax, ay] = RoughnessToAlpha(anisotropic, roughness, _hasAnisotropic);
    GGX specDistr{{ax, ay}, true};
    //微表面法线
    Vector3 wh = (si.Wi + wo * (reflect ? Float(1) : etaPath)).normalized();
    //确保微表面法线朝外部
    wh = MulSign(wh, Frame::CosTheta(wh));
    //电介质菲涅尔
    auto [frSpecDielectric, cosThetaT, etaIT, etaTI] = Fresnel::Dielectric(si.Wi.dot(wh), _eta);
    //检查微表面法线与宏观法线是否兼容
    bool canReflection = CheckMacMicNormal(wh, si.Wi, wo, cosThetaI, true);
    bool canRefraction = CheckMacMicNormal(wh, si.Wi, wo, cosThetaI, false);
    //检查各种lobe是否启用
    bool hasSpecReflectLobe = reflect && canReflection && (frSpecDielectric > Float(0));
    bool hasClearcoatLobe = _hasClearcoat && (clearcoat > Float(0)) && reflect && canReflection && isFrontSide;
    bool hasSpecTransLobe = _hasSpecTrans && (bsdf > Float(0)) && refract && canRefraction && (frSpecDielectric < Float(1));
    bool hasDiffuseLobe = (brdf > Float(0)) && reflect && isFrontSide;
    bool hasSheen = _hasSheen && (sheen > Float(0)) && reflect && (Float(1) - metallic > Float(0)) && isFrontSide;
    //微表面法线
    Float D = specDistr.D(wh);
    // smith's shadowing-masking
    Float G = specDistr.G(si.Wi, wo, wh);
    // BSDF最终评估的值
    Spectrum value(0);
    //高光反射部分
    if (hasSpecReflectLobe) {
      Float lum = _hasSpecTint ? baseColor.Luminance() : Float(1);
      Float specTint = _hasSpecTint ? _specTint->Eval(si) : Float(0);
      //菲涅尔项, disney使用Schlick近似
      Spectrum disneyF = DisneyFresnel(frSpecDielectric, metallic, specTint, baseColor, lum, si.Wi.dot(wh), isFrontSide, bsdf, _eta, _hasMetallic, _hasSpecTint);
      //这里不需要乘cosThetaO, 与渲染方程里的cosine项抵消了
      Spectrum val(disneyF * D * G / (Float(4) * std::abs(cosThetaI)));
      value += val;
    }
    //透射部分
    if (_hasSpecTrans && hasSpecTransLobe) {
      //伴随BSDF矫正
      Float scale = (context.Mode == TransportMode::Radiance) ? Sqr(invEtaPath) : Float(1);
      auto val = baseColor.cwiseSqrt() * bsdf *
                 std::abs((scale *
                           (Float(1) - frSpecDielectric) * D * G *
                           etaPath * etaPath * si.Wi.dot(wh) * wo.dot(wh)) /
                          (cosThetaI * Sqr(si.Wi.dot(wh) + etaPath * wo.dot(wh))));
      value += Spectrum(val);
    }
    //清漆部分
    if (_hasClearcoat && hasClearcoatLobe) {
      Float clearcoatGloss = _clearcoatGloss->Eval(si);
      Float ccF = SchlickF(Float(0.04), si.Wi.dot(wh), _eta);
      GTR1 ccDistr{Lerp(Float(0.1), Float(0.001), clearcoatGloss)};
      Float ccD = ccDistr.Eval(wh);
      Float ccG = ClearcoatG(si.Wi, wo, wh, Float(0.25));
      Spectrum val((clearcoat * Float(0.25)) * ccF * ccD * ccG * std::abs(cosThetaO));
      value += val;
    }
    //漫反射部分
    if (hasDiffuseLobe) {
      Float Fo = SchlickWeight(std::abs(cosThetaO));
      Float Fi = SchlickWeight(std::abs(cosThetaI));
      //漫反射
      Float fsDiff = (Float(1) - Float(0.5) * Fi) * (Float(1) - Float(0.5) * Fo);
      Float cosThetaD = wh.dot(wo);
      Float Rr = Float(2) * roughness * Sqr(cosThetaD);
      //逆射
      Float fsRetro = Rr * (Fo + Fi + Fo * Fi * (Rr - Float(1)));
      //次表面散射近似
      if (_hasFlatness) {
        //总之就是经验公式
        Float Fss90 = Rr / Float(2);
        Float Fss = Lerp(Float(1), Fss90, Fo) * Lerp(Float(1), Fss90, Fi);
        Float fsSSS = Float(1.25) * (Fss * (Float(1) / (std::abs(cosThetaO) + std::abs(cosThetaI)) - Float(0.5)) + Float(0.5));
        auto tmp = brdf * std::abs(cosThetaO) * baseColor * (1 / PI) * (Lerp(fsDiff + fsRetro, fsSSS, flatness));
        value += Spectrum(tmp);
      } else {
        Spectrum val(brdf * std::abs(cosThetaO) * baseColor * (1 / PI) * (fsDiff + fsRetro));
        value += val;
      }
      //光泽度
      if (_hasSheen && hasSheen) {
        Float Fd = SchlickWeight(std::abs(cosThetaD));
        if (_hasSheenTint) {
          Float sheenTint = _sheen_tint->Eval(si);
          Float lum = baseColor.Luminance();
          Spectrum cTint = (lum > Float(0) ? Spectrum(baseColor / lum) : Spectrum(1));
          Spectrum cSheen = LerpSpectrum(Spectrum(1), cTint, sheenTint);
          Spectrum val(sheen * (Float(1) - metallic) * Fd * cSheen * std::abs(cosThetaO));
          value += val;
        } else {
          Spectrum val(sheen * (Float(1) - metallic) * Fd * std::abs(cosThetaO));
          value += val;
        }
      }
    }
    return value;
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    Float cosThetaI = Frame::CosTheta(si.Wi);
    //忽略掉完美掠射 (grazing angle)
    if (cosThetaI == 0) {
      return 0;
    }
    //各种权重
    Float anisotropic = _hasAnisotropic ? _anisotropic->Eval(si) : Float(0);
    Float roughness = _roughness->Eval(si);
    Float specTrans = _hasSpecTrans ? _specTrans->Eval(si) : Float(0);
    Float metallic = _hasMetallic ? _metallic->Eval(si) : Float(0);
    Float clearcoat = _hasClearcoat ? _clearcoat->Eval(si) : Float(0);
    // BRDF与BSDF主要lobe的权重
    Float brdf = (Float(1) - metallic) * (Float(1) - specTrans);
    Float bsdf = (Float(1) - metallic) * specTrans;
    bool isFrontSide = cosThetaI > Float(0);
    Float etaPath = (isFrontSide ? _eta : Rcp(_eta));
    Float cosThetaO = Frame::CosTheta(wo);
    //判断当前方向
    bool reflect = cosThetaI * cosThetaO > Float(0);
    bool refract = cosThetaI * cosThetaO < Float(0);
    //微表面法线
    Vector3 wh = (si.Wi + wo * (reflect ? Float(1) : etaPath)).normalized();
    //确保微表面法线朝外部
    wh = MulSign(wh, Frame::CosTheta(wh));
    //主要高光
    auto [ax, ay] = RoughnessToAlpha(anisotropic, roughness, _hasAnisotropic);
    GGX specDistr{{ax, ay}, true};
    //电介质菲涅尔
    auto [frSpecDielectric, cosThetaT, etaIT, etaTI] = Fresnel::Dielectric(si.Wi.dot(wh), _eta);
    //计算lobe采样概率
    Float probSpecReflect = isFrontSide
                                ? _specularSample * (Float(1) - bsdf * (Float(1) - frSpecDielectric))
                                : frSpecDielectric;
    Float probSpecTrans = _hasSpecTrans
                              ? (isFrontSide
                                     ? _specularSample * bsdf * (Float(1) - frSpecDielectric)
                                     : (Float(1) - frSpecDielectric))
                              : Float(0);
    Float probClearcoat = _hasClearcoat
                              ? (isFrontSide
                                     ? Float(0.25) * clearcoat * _clearcoatSample
                                     : Float(0))
                              : Float(0);
    Float probDiffuse = (isFrontSide ? brdf * _diffuseReflectSample : Float(0));
    //归一化所有概率
    Float rcpTotalProb = Rcp(probSpecReflect + probSpecTrans + probClearcoat + probDiffuse);
    probSpecReflect *= rcpTotalProb;
    probSpecTrans *= rcpTotalProb;
    probClearcoat *= rcpTotalProb;
    probDiffuse *= rcpTotalProb;
    // dwh / dwo, 是微表面重要性采样时的jacobian项
    Float dwhdwoAbs;
    if (_hasSpecTrans) {
      Float wiDotH = si.Wi.dot(wh);
      Float woDotH = wo.dot(wh);
      dwhdwoAbs = std::abs(reflect
                               ? Rcp(Float(4) * woDotH)
                               : (Sqr(etaPath) * woDotH) / Sqr(wiDotH + etaPath * woDotH));
    } else {
      dwhdwoAbs = std::abs(Rcp(Float(4) * wo.dot(wh)));
    }
    //最终概率密度
    Float pdf = 0;
    //检查微表面法线与宏观法线是否兼容
    bool canReflection = CheckMacMicNormal(wh, si.Wi, wo, cosThetaI, true) && reflect;
    //微表面反射
    if (canReflection) {
      pdf += probSpecReflect * specDistr.Pdf(MulSign(si.Wi, cosThetaI), wh) * dwhdwoAbs;
    }
    //漫反射
    if (reflect) {
      pdf += probDiffuse * Warp::SquareToCosineHemispherePdf(wo);
    }
    //微表面透射
    if (_hasSpecTrans) {
      bool canRefrection = CheckMacMicNormal(wh, si.Wi, wo, cosThetaI, false) && refract;
      if (canRefrection) {
        pdf += probSpecTrans * specDistr.Pdf(MulSign(si.Wi, cosThetaI), wh) * dwhdwoAbs;
      }
    }
    //清漆
    if (_hasClearcoat) {
      Float clearcoatGloss = _clearcoatGloss->Eval(si);
      GTR1 ccDistr{Lerp(Float(0.1), Float(0.001), clearcoatGloss)};
      if (canReflection) {
        pdf += probClearcoat * ccDistr.Pdf(wh) * dwhdwoAbs;
      }
    }
    return pdf;
  }

 private:
  static bool ConfigHasValue(const std::string& name, const ConfigNode& cfg) {
    if (cfg.HasNode(name)) {
      ConfigNode node = cfg.Read<ConfigNode>(name);
      if (node.data->is_number()) {
        Float v = node.data->get<Float>();
        return v != 0;
      } else {
        return true;
      }
    } else {
      return false;
    }
  }

  /**
   * @brief 计算roughness到alpha, 同时考虑各项异性
   */
  static std::pair<Float, Float> RoughnessToAlpha(
      Float anisotropic,
      Float roughness,
      bool hasAnisotropic) {
    Float roughness2 = Sqr(roughness);
    if (!hasAnisotropic) {
      Float a = std::max(Float(0.001), roughness2);
      return {a, a};
    }
    Float aspect = std::sqrt(Float(1) - Float(0.9) * anisotropic);
    return {std::max(Float(0.001), roughness2 / aspect),
            std::max(Float(0.001), roughness2 * aspect)};
  }

  static bool CheckMacMicNormal(
      const Vector3& m,
      const Vector3& wi,
      const Vector3& wo,
      const Float& cosThetaI,
      bool reflection) {
    if (reflection) {
      return (wi.dot(MulSign(m, cosThetaI)) > Float(0)) &&
             (wo.dot(MulSign(m, cosThetaI)) > Float(0));
    } else {
      return (wi.dot(MulSign(m, cosThetaI)) > Float(0)) &&
             (wo.dot(cosThetaI >= 0 ? -m : m) > Float(0));
    }
  }

  static Float SchlickWeight(Float cosI) {
    Float m = std::clamp(Float(1) - cosI, Float(0), Float(1));
    return Sqr(Sqr(m)) * m;
  }

  /**
   * @brief Schlick近似菲涅尔反射系数 F = R0 + (1 - R0)(1-cos^5(i)), 不带颜色
   *
   * @param R0 入射叫与微表面法线相同时, 菲涅尔值
   * @param cosThetaI 入射角
   * @param eta 透射率
   */
  static Float SchlickF(Float R0, Float cosThetaI, Float eta) {
    bool isOutside = cosThetaI >= Float(0);
    Float rcpEta = Rcp(eta);
    Float etaIT = (isOutside ? eta : rcpEta);
    Float etaTI = (isOutside ? rcpEta : eta);
    Float sqrCosThetaT = Fmadd(-Fmadd(-cosThetaI, cosThetaI, Float(1)), Sqr(etaTI), Float(1));
    Float cosThetaT = SafeSqrt(sqrCosThetaT);
    return etaIT > Float(1)
               ? Lerp(SchlickWeight(std::abs(cosThetaI)), Float(1), R0)
               : Lerp(SchlickWeight(cosThetaT), Float(1), R0);
  }

  /**
   * @brief Schlick近似菲涅尔反射系数 F = R0 + (1 - R0)(1-cos^5(i)), 带颜色
   *
   * @param R0 入射叫与微表面法线相同时, 菲涅尔值
   * @param cosThetaI 入射角
   * @param eta 透射率
   */
  static Spectrum SchlickF(const Spectrum& R0, Float cosThetaI, Float eta) {
    bool isOutside = cosThetaI >= Float(0);
    Float rcpEta = Rcp(eta);
    Float etaIT = (isOutside ? eta : rcpEta);
    Float etaTI = (isOutside ? rcpEta : eta);
    Float sqrCosThetaT = Fmadd(-Fmadd(-cosThetaI, cosThetaI, Float(1)), Sqr(etaTI), Float(1));
    Float cosThetaT = SafeSqrt(sqrCosThetaT);
    return etaIT > Float(1)
               ? LerpSpectrum(SchlickWeight(std::abs(cosThetaI)), Float(1), R0)
               : LerpSpectrum(SchlickWeight(cosThetaT), Float(1), R0);
  }

  static Float SchlickR0Eta(Float eta) {
    return Sqr((eta - Float(1)) / (eta + Float(1)));
  }

  /**
   * @brief 融合金属与电介质菲涅尔, 使用Schlick近似specTint与金属, 电介质则使用真正的菲涅尔
   *
   * @param dielectricF 真正的菲涅尔
   * @param metallic metallic参数
   * @param specTint spec_tint参数
   * @param baseColor base_color参数
   * @param lum 亮度
   * @param cosThetaI 入射角
   * @param isFrontSide 是不是在外侧
   * @param bsdf 透射权重
   * @param eta 透射率
   * @param hasMetallic 是否存在metallic参数
   * @param hasSpecTint 是否存在spec_tint参数
   * @return Spectrum
   */
  static Spectrum DisneyFresnel(
      const Float& dielectricF,
      const Float& metallic, const Float& specTint, const Spectrum& baseColor,
      const Float& lum, const Float& cosThetaI,
      bool isFrontSide,
      const Float& bsdf, const Float& eta,
      bool hasMetallic, bool hasSpecTint) {
    bool isOutside = cosThetaI >= Float(0);
    Float rcpEta = Rcp(eta);
    Float etaIT = (isOutside ? eta : rcpEta);
    Spectrum schlickF(0);
    // 金属使用Schlick近似
    if (hasMetallic) {
      schlickF += metallic * SchlickF(baseColor, cosThetaI, eta);
    }
    // tint也使用Schlick近似
    if (hasSpecTint) {
      Spectrum cTint = (lum > Float(0) ? Spectrum(baseColor / lum) : Spectrum(1));
      Spectrum F0_spec_tint(cTint * SchlickR0Eta(etaIT));
      schlickF += (Float(1) - metallic) * specTint * SchlickF(F0_spec_tint, cosThetaI, eta);
    }
    // 外部的菲涅尔结果
    Spectrum frontF = Spectrum(Spectrum((Float(1) - metallic) * (Float(1) - specTint) * dielectricF) + schlickF);
    //内部没有金属度和tint, 返回普通的电介质菲涅尔
    return (isFrontSide ? frontF : bsdf * dielectricF);
  }

  static Float ClearcoatG(const Vector3& wi, const Vector3& wo, const Vector3& wh, const Float& alpha) {
    return SmithGGX1(wi, wh, alpha) * SmithGGX1(wo, wh, alpha);
  }

  static Float SmithGGX1(const Vector3& v, const Vector3& wh, const Float& alpha) {
    Float alpha2 = Sqr(alpha);
    Float cosTheta = std::abs(Frame::CosTheta(v));
    Float cosTheta2 = Sqr(cosTheta);
    Float tanTheta2 = (Float(1) - cosTheta2) / cosTheta2;
    Float result = Float(2) * Rcp(Float(1) + std::sqrt(Float(1) + alpha2 * tanTheta2));
    // 垂直入射
    if (Frame::CosTheta(v) == 1) {
      result = 1;
    }
    if (v.dot(wh) * Frame::CosTheta(v) <= Float(0)) {
      result = Float(0);
    }
    return result;
  }

  Unique<TextureRGB> _baseColor;
  Unique<TextureGray> _roughness;
  Unique<TextureGray> _anisotropic;
  Unique<TextureGray> _sheen;
  Unique<TextureGray> _sheen_tint;
  Unique<TextureGray> _specTrans;
  Unique<TextureGray> _flatness;
  Unique<TextureGray> _specTint;
  Unique<TextureGray> _clearcoat;
  Unique<TextureGray> _clearcoatGloss;
  Unique<TextureGray> _metallic;
  Float _eta;
  Float _specular;
  bool _hasEtaSpecular;

  Float _diffuseReflectSample;
  Float _specularSample;
  Float _clearcoatSample;

  bool _hasClearcoat;
  bool _hasSheen;
  bool _hasSpecTrans;
  bool _hasMetallic;
  bool _hasSpecTint;
  bool _hasSheenTint;
  bool _hasAnisotropic;
  bool _hasFlatness;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(Disney, "disney");
