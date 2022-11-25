#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

class RAD_EXPORT_API PhaseFunction {
 public:
  virtual ~PhaseFunction() noexcept = default;

  virtual std::pair<Vector3, Float> Sample(const MediumInteraction& mi, const Vector2& xi) const = 0;
  virtual Float Eval(const MediumInteraction& mi, const Vector3& wo) const = 0;
};

}  // namespace Rad
