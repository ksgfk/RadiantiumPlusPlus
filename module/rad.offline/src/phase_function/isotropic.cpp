#include <rad/offline/render/phase_function.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/warp.h>

namespace Rad {

/**
 * @brief 各向同性相函数，均匀散射到空间任何一个方向
 */
class IsotropicPhase final : public PhaseFunction {
 public:
  IsotropicPhase(BuildContext* ctx, const ConfigNode& cfg) {}

  std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) const {
    Vector3 wo = Warp::SquareToUniformSphere(xi);
    Float pdf = Warp::SquareToUniformSpherePdf();
    return std::make_pair(wo, pdf);
  }

  Float Eval(const MediumInteraction& mi, const Vector3& wo) const {
    return Warp::SquareToUniformSpherePdf();
  }
};

class IsotropicPhaseFactory : public PhaseFunctionFactory {
 public:
  IsotropicPhaseFactory() : PhaseFunctionFactory("isotropic_phase") {}
  ~IsotropicPhaseFactory() noexcept override = default;
  Unique<PhaseFunction> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<IsotropicPhase>(ctx, cfg);
  }
};

Unique<PhaseFunctionFactory> _FactoryCreateIsotropicPhaseFunc_() {
  return std::make_unique<IsotropicPhaseFactory>();
}

}  // namespace Rad
