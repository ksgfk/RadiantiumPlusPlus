#pragma once

#include "radiantium.h"
#include <utility>

namespace rad::math {

Float Sqr(Float v);
Float Sign(Float v);
// mul sign of v2 to v1
Float MulSign(Float v1, Float v2);
Float Rcp(Float v);
std::pair<Vec3, Vec3> CoordinateSystem(Vec3 n);
Mat4 LookAtLeftHand(const Vec3& origin, const Vec3& target, const Vec3& up);

}  // namespace rad::math
