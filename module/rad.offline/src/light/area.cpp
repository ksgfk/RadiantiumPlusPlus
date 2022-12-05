#include <rad/offline/render/light.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/shape.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/distribution.h>
#include <rad/offline/warp.h>

namespace Rad {

/**
 * @brief 区域光, 从任意形状表面发出照明的光源, 无论观察者在哪, 它都发出均匀的辐射
 * 由于它的表面积不是0, 通常可以投影出软阴影
 */
class DiffuseArea final : public Light {
 public:
  DiffuseArea(BuildContext* ctx, const ConfigNode& cfg) {
    _radiance = ConfigNodeReadTexture(ctx, cfg, "radiance", Color24f(1));
    UInt32 width = _radiance->Width(), height = _radiance->Height();
    _isUniform = width == 1 && height == 1;
    _flag = (UInt32)LightType::Surface;
    if (!_isUniform) {
      std::vector<Float> luminance;  //计算光源亮度, 用来重要性采样
      luminance.resize(width * height);
      for (UInt32 y = 0; y < height; y++) {
        Float v = y / (Float)height;
        UInt32 start = y * width;
        for (UInt32 x = 0; x < width; x++) {
          Float u = x / (Float)width;
          SurfaceInteraction si;
          si.UV = Vector2(u, v);
          Color24f color = _radiance->Eval(si);
          Float lum = color.Luminance();
          luminance[start + x] = lum;
        }
      }
      _dist = ContinuousDistribution2D(luminance.data(), width, height);
    }
  }
  ~DiffuseArea() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    Color24f radiance = si.ToWorld(si.Wi).dot(si.N) >= 0 ? _radiance->Eval(si) : Color24f(0);
    return Color24fToSpectrum(radiance);
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    DirectionSampleResult dsr{};
    Color24f radiance;
    if (_isUniform) {
      dsr = _shape->SampleDirection(ref, xi);
      radiance = (-dsr.Dir).dot(dsr.N) >= 0
                     ? _radiance->Eval(SurfaceInteraction(dsr))
                     : Color24f(0);
    } else {
      auto [index, _, pdf] = _dist.Sample(xi);
      if (pdf <= 0) {
        return {{}, Spectrum(0)};
      }
      Float y = index.y() / (Float)_radiance->Height();
      Float x = index.x() / (Float)_radiance->Width();
      Vector2 uv(x, y);
      SurfaceInteraction surface = _shape->EvalParamSurface(uv);
      dsr = surface.ToDsr(ref);
      radiance = (-dsr.Dir).dot(dsr.N) >= 0
                     ? _radiance->Eval(surface)
                     : Color24f(0);
    }
    return std::make_pair(dsr, Color24fToSpectrum(radiance));
  }

  Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const override {
    if (_isUniform) {
      Float pdf = (-dsr.Dir).dot(dsr.N) >= 0 ? _shape->PdfDirection(ref, dsr) : 0;
      return pdf;
    } else {
      SurfaceInteraction si = _shape->EvalParamSurface(dsr.UV);
      Float pdf = _dist.Pdf(dsr.UV);
      if (pdf <= 0) {
        return 0;
      }
      Float dp = dsr.Dir.dot(dsr.N);
      Float result = pdf * Math::Sqr(dsr.Dist) / (si.dPdU.cross(si.dPdV).norm() * -dp);
      return result;
    }
  }

  std::pair<PositionSampleResult, Float> SamplePosition(const Vector2& xi) const override {
    if (_isUniform) {
      PositionSampleResult psr = _shape->SamplePosition(xi);
      Float weight = psr.Pdf > 0 ? psr.Pdf : 0;
      return std::make_pair(psr, weight);
    } else {
      auto [index, _, pdf] = _dist.Sample(xi);
      if (pdf <= 0) {
        return {{}, Float(0)};
      }
      Float y = index.y() / (Float)_radiance->Height();
      Float x = index.x() / (Float)_radiance->Width();
      Vector2 uv(x, y);
      SurfaceInteraction surface = _shape->EvalParamSurface(uv);
      pdf /= (surface.dPdU.cross(surface.dPdV).norm());
      PositionSampleResult psr = surface.ToPsr();
      psr.Pdf = pdf;
      psr.IsDelta = false;
      Float weight = psr.Pdf > 0 ? psr.Pdf : 0;
      return std::make_pair(psr, weight);
    }
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    return _shape->PdfPosition(psr);
  }

  std::pair<Ray, Spectrum> SampleRay(const Vector2& xi2, const Vector2& xi3) const override {
    auto [psr, pdf] = SamplePosition(xi2);
    Vector3 local = Warp::SquareToCosineHemisphere(xi3);
    SurfaceInteraction si(psr);
    Ray ray = si.SpawnRay(si.ToWorld(local));
    Spectrum eval = Eval(si);
    auto li = eval * Math::PI / pdf;
    return std::make_pair(ray, Spectrum(li));
  }

  std::tuple<Ray, Spectrum, PositionSampleResult, Float, Float> SampleLe(
      const Vector2& xi2,
      const Vector2& xi3) const override {
    auto [psr, pdfPos] = SamplePosition(xi2);
    Vector3 local = Warp::SquareToCosineHemisphere(xi3);
    Float pdfDir = Warp::SquareToCosineHemispherePdf(local);
    SurfaceInteraction si(psr);
    Ray ray = si.SpawnRay(si.ToWorld(local));
    Spectrum le = Eval(si);
    return std::make_tuple(ray, le, psr, pdfPos, pdfDir);
  }

  std::pair<Float, Float> PdfLe(const PositionSampleResult& psr, const Vector3& dir) const override {
    SurfaceInteraction si(psr);
    Float pdfPos = PdfPosition(psr);
    Float pdfDir = Warp::SquareToCosineHemispherePdf(si.ToLocal(dir));
    return std::make_pair(pdfPos, std::abs(pdfDir));
  }

 private:
  Unique<TextureRGB> _radiance;
  bool _isUniform;
  ContinuousDistribution2D _dist;
};

class DiffuseAreaFactory final : public LightFactory {
 public:
  DiffuseAreaFactory() : LightFactory("area") {}
  ~DiffuseAreaFactory() noexcept override = default;
  Unique<Light> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<DiffuseArea>(ctx, cfg);
  }
};

Unique<LightFactory> _FactoryCreateDiffuseAreaFunc_() {
  return std::make_unique<DiffuseAreaFactory>();
}

}  // namespace Rad
