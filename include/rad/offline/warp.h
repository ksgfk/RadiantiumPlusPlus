#pragma once

#include "types.h"

namespace Rad::Warp {

Vector3 SquareToUniformHemisphere(const Vector2& u);
Float SquareToUniformHemispherePdf();

Vector2 SquareToUniformDisk(const Vector2& u);
Float SquareToUniformDiskPdf();

Vector3 SquareToCosineHemisphere(const Vector2& u);
Float SquareToCosineHemispherePdf(const Vector3& v);

Vector2 SquareToUniformTriangle(const Vector2& u);
Float SquareToUniformTrianglePdf();

}  // namespace Rad::Warp
