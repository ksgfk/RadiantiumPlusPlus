#pragma once

#include "radiantium.h"
#include <utility>

namespace rad::math {

Float Sqr(Float v);
Float Sign(Float v);
Float MulSign(Float v1, Float v2); // mul sign of v2 to v1
Float Rcp(Float v);
Float Fmadd(Float a, Float b, Float c);
Vec3 Fmadd(Vec3 a, Vec3 b, Vec3 c);
Float Rsqrt(Float v);

std::pair<Vec3, Vec3> CoordinateSystem(Vec3 n);

Mat4 LookAtLeftHand(const Vec3& origin, const Vec3& target, const Vec3& up);

}  // namespace rad::math
