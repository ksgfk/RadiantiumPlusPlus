#pragma once

#include "radiantium.h"

namespace rad {

struct Ray {
  Vec3 o;
  Vec3 d;
  Float minT;
  Float maxT;

  Vec3 operator()(Float t) const;
};

}  // namespace rad
