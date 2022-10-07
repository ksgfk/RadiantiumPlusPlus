#include <rad/offline/render/phase_function.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

class IsotropicPhase final : public PhaseFunction {
 public:
  IsotropicPhase(BuildContext* ctx, const ConfigNode& cfg) {}

  std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) {
    Vector3 wo = Warp::SquareToUniformSphere(xi);
    Float pdf = Warp::SquareToUniformSpherePdf();
    return std::make_pair(wo, pdf);
  }

  Float Eval(const MediumInteraction& mi, const Vector3& wo) {
    return Warp::SquareToUniformSpherePdf();
  }
};

}  // namespace Rad

RAD_FACTORY_PHASEFUNCTION_DECLARATION(IsotropicPhase, "isotropic_phase");
