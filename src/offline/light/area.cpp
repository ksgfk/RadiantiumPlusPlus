#include <rad/offline/render/light.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/utility/distribution.h>

namespace Rad {

/**
 * @brief 区域光, 从任意形状表面发出照明的光源, 无论观察者在哪, 它都发出均匀的辐射
 * 由于它的表面积不是0, 通常可以投影出软阴影
 */
class DiffuseArea final : public Light {
 public:
  DiffuseArea(BuildContext* ctx, const ConfigNode& cfg) {
    _radiance = cfg.ReadTexture(*ctx, "radiance", Color(1));
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
          Color color = _radiance->Eval(si);
          Float lum = color.Luminance();
          luminance[start + x] = lum;
        }
      }
      _dist = ContinuousDistribution2D(luminance.data(), width, height);
    }
  }
  ~DiffuseArea() noexcept override = default;

  Spectrum Eval(const SurfaceInteraction& si) const override {
    Color radiance = si.ToWorld(si.Wi).dot(si.N) >= 0 ? _radiance->Eval(si) : Color(0);
    return Spectrum(radiance);
  }

  std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const override {
    DirectionSampleResult dsr{};
    Color radiance;
    if (_isUniform) {
      dsr = _shape->SampleDirection(ref, xi);
      radiance = (-dsr.Dir).dot(dsr.N) >= 0
                     ? _radiance->Eval(SurfaceInteraction(dsr))
                     : Color(0);
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
                     : Color(0);
    }
    return std::make_pair(dsr, Spectrum(radiance));
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

 private:
  Unique<Texture<Color>> _radiance;
  bool _isUniform;
  ContinuousDistribution2D _dist;
};

}  // namespace Rad

RAD_FACTORY_LIGHT_DECLARATION(DiffuseArea, "area");
