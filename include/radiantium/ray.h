#pragma once

#include "radiantium.h"

namespace rad {

struct Ray {
  Vec3 O;
  Vec3 D;
  Float MinT;
  Float MaxT;

  Vec3 operator()(Float t) const;
};

}  // namespace rad
