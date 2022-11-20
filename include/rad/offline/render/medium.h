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
  virtual std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const = 0;

  MediumInteraction SampleInteraction(const Ray& ray, Float sample, UInt32 channel) const;
  std::pair<Spectrum, Spectrum> EvalTrAndPdf(const MediumInteraction& mi, const SurfaceInteraction& si) const;

  const PhaseFunction& GetPhaseFunction() const noexcept { return *_phaseFunction.get(); }
  bool IsHomogeneous() const { return _isHomogeneous; }
  bool HasSpectralExtinction() const { return _hasSpectralExtinction; }

 protected:
  Unique<PhaseFunction> _phaseFunction;
  bool _isHomogeneous;
  bool _hasSpectralExtinction;
};

}  // namespace Rad
