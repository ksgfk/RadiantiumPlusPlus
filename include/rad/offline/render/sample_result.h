#pragma once

#include "../common.h"

namespace Rad {

struct PositionSampleResult {
  Vector3 P;
  Vector3 N;
  Vector2 UV;
  Float Pdf;
  bool IsDelta;
};

struct DirectionSampleResult : PositionSampleResult {
  Vector3 Dir;
  Float Dist;
};

}  // namespace Rad
