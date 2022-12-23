#include <rad/offline/render/phase_function.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/math_ext.h>

namespace Rad {

class HenyeyGreenstein final : public PhaseFunction {
 public:
  HenyeyGreenstein(BuildContext* ctx, const ConfigNode& cfg) {
    _g = cfg.ReadOrDefault("g", Float(0.8));
    if (_g <= -1 || _g >= 1) {
      throw RadArgumentException("param g must lie in the interval (-1, 1)");
    }
  }

  std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) const {
    Float sqrTerm = (1 - _g * _g) / (1 - _g + 2 * _g * xi.x());
    Float cosTheta = (1 + _g * _g - sqrTerm * sqrTerm) / (2 * _g);
    if (std::abs(_g) < Math::Epsilon<Float>()) {
      cosTheta = 1 - 2 * xi.x();
    }
    Float sinTheta = Math::SafeSqrt(1 - cosTheta * cosTheta);
    auto [sinPhi, cosPhi] = Math::SinCos(2 * Math::PI * xi.y());
    Vector3f wo(sinTheta * cosPhi, sinTheta * sinPhi, -cosTheta);
    wo = mi.ToWorld(wo);
    Float pdf = EvalHG(-cosTheta);
    return std::make_pair(wo, pdf);
  }

  Float Eval(const MediumInteraction& mi, const Vector3& wo) const {
    return EvalHG(wo.dot(mi.Wi));
  }

 private:
  Float EvalHG(Float cosTheta) const {
    Float temp = Float(1) + _g * _g + Float(2) * _g * cosTheta;
    return (1 / (4 * Math::PI)) * (1 - _g * _g) / (temp * std::sqrt(temp));
  }

  Float _g;
};

class HenyeyGreensteinFactory : public PhaseFunctionFactory {
 public:
  HenyeyGreensteinFactory() : PhaseFunctionFactory("hg_phase") {}
  ~HenyeyGreensteinFactory() noexcept override = default;
  Unique<PhaseFunction> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<HenyeyGreenstein>(ctx, cfg);
  }
};

Unique<PhaseFunctionFactory> _FactoryCreateHenyeyGreensteinFunc_() {
  return std::make_unique<HenyeyGreensteinFactory>();
}

}  // namespace Rad
