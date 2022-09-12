#include <radiantium/warp.h>

#include <cmath>
#include <radiantium/math_ext.h>

using namespace rad::math;

namespace rad::warp {

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90unifom-hemisphere
Vec3 SquareToUniformHemisphere(const Vec2& u) {
  Float z = u[0];
  Float r = std::sqrt(std::max(Float(0), 1 - Sqr(z)));
  Float phi = 2 * PI * u[1];
  auto [sinPhi, cosPhi] = SinCos(phi);
  return Vec3(r * cosPhi, r * sinPhi, z);
}
float SquareToUniformHemispherePdf() {
  return 1 / (2 * PI);
}

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90unifom-disk-%E6%AD%A3%E5%BC%8F%E8%A7%A3%E5%86%B3%E6%96%B9%E6%A1%88
Vec2 SquareToUniformDisk(const Vec2& u) {
  Float r = std::sqrt(u[0]);
  Float angle = 2 * PI * u[1];
  auto [s, c] = SinCos(angle);
  return Vec2(r * c, r * s);
}
Float SquareToUniformDiskPdf() {
  return 1 / PI;
}

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90diffuse-brdf
Vec3 SquareToCosineHemisphere(const Vec2& u) {
#if 0
  Float sinTheta = std::sqrt(1 - u.x());
  Float cosTheta = sqrt(u.x());
  Float phi = 2 * PI * u.y();
  auto [sinPhi, cosPhi] = SinCos(phi);
  return Vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
#else
  Vec2 bottom = SquareToUniformDisk(u);
  Float x = bottom.x();
  Float y = bottom.y();
  return Vec3(x, y, std::sqrt(1 - Sqr(x) - Sqr(y)));
#endif
}
float SquareToCosineHemispherePdf(const Vec3& v) {
  return v.z() * (1 / PI);
}

}  // namespace rad::warp
