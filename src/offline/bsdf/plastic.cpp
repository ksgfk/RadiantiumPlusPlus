#include <rad/offline/render/bsdf.h>

#include <rad/offline/build.h>
#include <rad/offline/asset.h>
#include <rad/offline/render.h>
#include <rad/offline/render/fresnel.h>

namespace Rad {

class Plastic final : public Bsdf {
 public:
  Plastic(BuildContext* ctx, const ConfigNode& cfg) {
    _flags = BsdfType::Diffuse | BsdfType::Reflection | BsdfType::Delta;
    Float intIor = cfg.ReadOrDefault("int_ior", Float(1.49f));      //内部折射率
    Float extIor = cfg.ReadOrDefault("ext_ior", Float(1.000277f));  //外部折射率
    _eta = intIor / extIor;
    _diffuse = cfg.ReadTexture(*ctx, "diffuse", Color(Float(0.5)));
    _specular = cfg.ReadTexture(*ctx, "specular", Color(Float(1)));
    _noLinear = cfg.ReadOrDefault("no_linear", false);
    _fdrInt = Fresnel::DiffuseReflectance(Float(1) / _eta);
    _invEta2 = Float(1) / (_eta * _eta);

    Float64 diffMean = 0;
    for (UInt32 y = 0; y < _diffuse->Height(); y++) {
      for (UInt32 x = 0; x < _diffuse->Width(); x++) {
        diffMean += _diffuse->Read(x, y).Luminance();
      }
    }
    diffMean /= (_diffuse->Height() * _diffuse->Width());
    Float64 specMean = 0;
    for (UInt32 y = 0; y < _specular->Height(); y++) {
      for (UInt32 x = 0; x < _specular->Width(); x++) {
        specMean += _specular->Read(x, y).Luminance();
      }
    }
    specMean /= (_specular->Height() * _specular->Width());
    _specularSampleWeight = (Float)(specMean / (diffMean + specMean));
  }
  ~Plastic() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    if (!context.IsEnable(BsdfType::Reflection)) {
      return {{}, Spectrum(0)};
    }
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return {{}, Spectrum(0)};
    }
    Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
    Float probSpec = fi * _specularSampleWeight;
    Float probDiff = (1 - fi) * (1 - _specularSampleWeight);
    if (context.IsEnable(BsdfType::Delta) && context.IsEnable(BsdfType::Diffuse)) {
      probSpec = probSpec / (probSpec + probDiff);
    } else {
      probSpec = context.IsEnable(BsdfType::Delta) ? Float(1) : Float(0);
    }
    probDiff = 1 - probSpec;
    BsdfSampleResult bsr{};
    bsr.Eta = 1;
    Spectrum f;
    if (lobeXi < probSpec) {
      bsr.Wo = Fresnel::Reflect(si.Wi);
      bsr.Pdf = probSpec;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Delta;
      Spectrum spec = _specular->Eval(si);
      f = Spectrum(spec * fi);
    } else {
      bsr.Wo = Warp::SquareToCosineHemisphere(dirXi);
      bsr.Pdf = Warp::SquareToCosineHemispherePdf(bsr.Wo) * probDiff;
      bsr.TypeMask = BsdfType::Reflection | BsdfType::Diffuse;
      Float cosThetaO = Frame::CosTheta(bsr.Wo);
      Float fo = std::get<0>(Fresnel::Dielectric(cosThetaO, Float(_eta)));
      Spectrum diff = _diffuse->Eval(si);
      auto num = diff * (1 / Math::PI) * _invEta2 * (1 - fi) * (1 - fo) * cosThetaO;
      if (_noLinear) {
        f = Spectrum(num.cwiseProduct((Spectrum::Constant(1) - (diff * _fdrInt)).cwiseInverse()));
      } else {
        f = Spectrum(num / (1 - _fdrInt));
      }
    }
    return std::make_pair(bsr, f);
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
    Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
    Float fo = std::get<0>(Fresnel::Dielectric(cosThetaO, Float(_eta)));
    Spectrum diff = _diffuse->Eval(si);
    auto num = diff * (1 / Math::PI) * _invEta2 * (1 - fi) * (1 - fo) * cosThetaO;
    Spectrum f;
    if (_noLinear) {
      f = Spectrum(num.cwiseProduct((Spectrum::Constant(1) - (diff * _fdrInt)).cwiseInverse()));
    } else {
      f = Spectrum(num / (1 - _fdrInt));
    }
    return f;
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
    Float probDiff = 1;
    if (context.IsEnable(BsdfType::Delta)) {
      Float fi = std::get<0>(Fresnel::Dielectric(cosThetaI, _eta));
      Float probSpec = fi * _specularSampleWeight;
      probDiff = (1 - fi) * (1 - _specularSampleWeight);
      probDiff = probDiff / (probDiff + probSpec);
    }
    Float pdf = Warp::SquareToCosineHemispherePdf(wo) * probDiff;
    return pdf;
  }

 private:
  Float _eta;
  Unique<TextureRGB> _diffuse;
  Unique<TextureRGB> _specular;
  bool _noLinear;
  Float _fdrInt;
  Float _specularSampleWeight;
  Float _invEta2;
};

}  // namespace Rad

RAD_FACTORY_BSDF_DECLARATION(Plastic, "plastic");
