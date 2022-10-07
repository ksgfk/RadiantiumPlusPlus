#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

class PhaseFunction {
 public:
  virtual ~PhaseFunction() noexcept = default;

  virtual std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) = 0;
  virtual Float Eval(const MediumInteraction& mi, const Vector3& wo) = 0;
};

}  // namespace Rad
