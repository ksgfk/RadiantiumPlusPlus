#include "radiantium.h"

namespace rad::warp {

Vec3 SquareToUniformHemisphere(const Vec2& u);
Float SquareToUniformHemispherePdf();

Vec2 SquareToUniformDisk(const Vec2& u);
Float SquareToUniformDiskPdf();

Vec3 SquareToCosineHemisphere(const Vec2& u);
Float SquareToCosineHemispherePdf(const Vec3& v);

}
