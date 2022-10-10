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

  virtual std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const = 0;
  virtual Spectrum GetMajorant(const MediumInteraction& mi) const = 0;
  virtual std::tuple<Spectrum, Spectrum, Spectrum> GetScattingCoeff(const MediumInteraction& mi) const = 0;

  MediumInteraction SampleInteraction(const Ray& ray, Float xi, UInt32 channel) const;
  std::pair<Spectrum, Spectrum> EvalTrAndPdf(const MediumInteraction& mi, const SurfaceInteraction& si) const;

  const PhaseFunction* GetPhase() const { return _phaseFunction.get(); }
  bool IsHomogeneous() const { return _isHomogeneous; }

 protected:
  Unique<PhaseFunction> _phaseFunction;
  bool _isHomogeneous = false;
};

}  // namespace Rad
