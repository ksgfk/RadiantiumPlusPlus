#pragma once

#include "../common.h"
#include "interaction.h"
#include "phase_function.h"

#include <utility>

namespace Rad {

class Medium {
 public:
  Medium(BuildContext* ctx, const ConfigNode& cfg);
  virtual ~Medium() noexcept = default;

  virtual Spectrum Tr(const Ray& ray, Sampler& sampler) const = 0;
  virtual std::pair<MediumInteraction, Spectrum> Sample(const Ray& ray, Sampler& sampler) const = 0;

  const PhaseFunction& GetPhaseFunction() const noexcept { return *_phaseFunction.get(); }

 protected:
  Unique<PhaseFunction> _phaseFunction;
};

}  // namespace Rad
