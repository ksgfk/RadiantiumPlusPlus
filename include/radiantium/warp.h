#include "radiantium.h"

namespace rad::warp {

Vec3 SquareToUniformHemisphere(const Vec2& sample);
float SquareToUniformHemispherePdf();

Vec3 SquareToCosineHemisphere(const Vec2& sample);
float SquareToCosineHemispherePdf(const Vec3& v);

}
