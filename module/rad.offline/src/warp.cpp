#include <rad/offline/warp.h>

#include <rad/offline/math_ext.h>

using namespace Rad::Math;

namespace Rad::Warp {

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90unifom-hemisphere
Vector3 SquareToUniformHemisphere(const Vector2& u) {
  Float z = u[0];
  Float r = std::sqrt(std::max(Float(0), 1 - Sqr(z)));
  Float phi = 2 * PI * u[1];
  auto [sinPhi, cosPhi] = SinCos(phi);
  return Vector3(r * cosPhi, r * sinPhi, z);
}
Float SquareToUniformHemispherePdf() {
  return 1 / (2 * PI);
}

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90unifom-disk-%E6%AD%A3%E5%BC%8F%E8%A7%A3%E5%86%B3%E6%96%B9%E6%A1%88
Vector2 SquareToUniformDisk(const Vector2& u) {
  Float r = std::sqrt(u[0]);
  Float angle = 2 * PI * u[1];
  auto [s, c] = SinCos(angle);
  return Vector2(r * c, r * s);
}
Float SquareToUniformDiskPdf() {
  return 1 / PI;
}

// https://ksgfk.github.io/2022/09/08/MonteCarlo/#%E4%BE%8B%E5%AD%90diffuse-brdf
Vector3 SquareToCosineHemisphere(const Vector2& u) {
#if 0
  Float sinTheta = std::sqrt(1 - u.x());
  Float cosTheta = sqrt(u.x());
  Float phi = 2 * PI * u.y();
  auto [sinPhi, cosPhi] = SinCos(phi);
  return Vector3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
#else
  Vector2 bottom = SquareToUniformDisk(u);
  Float x = bottom.x();
  Float y = bottom.y();
  Float z = SafeSqrt(1 - Sqr(x) - Sqr(y));
  return Vector3(x, y, z);
#endif
}
Float SquareToCosineHemispherePdf(const Vector3& v) {
  return v.z() * (1 / PI);
}

Vector2 SquareToUniformTriangle(const Vector2& u) {
  Float t = SafeSqrt(1 - u.x());
  return Vector2(1 - t, t * u.y());
}
Float SquareToUniformTrianglePdf() {
  return 2;
}

Vector3 SquareToUniformSphere(const Vector2& u) {
  Float z = Fmadd(-2, u.y(), 1);
  Float r = SafeSqrt(Fmadd(-z, z, 1));
  auto [s, c] = SinCos(2 * PI * u.x());
  return Vector3(r * c, r * s, z);
}
Float SquareToUniformSpherePdf() {
  return 1 / (4 * PI);
}

Vector3 SquareToUniformCone(const Vector2& sample, Float cosCutoff) {
  Float cosTheta = (1 - sample.y()) + sample.y() * cosCutoff;
  Float sinTheta = SafeSqrt(Fmadd(-cosTheta, cosTheta, 1));
  auto [s, c] = SinCos(2 * PI * sample.x());
  return Vector3(c * sinTheta, s * sinTheta, cosTheta);
}
Float SquareToUniformConePdf(Float cosCutoff) {
  return 1 / (2 * PI) / (1 - cosCutoff);
}

Vector2 SquareToUniformDiskConcentric(const Vector2& u) {
  Float x = Fmadd(2, u.x(), -1);
  Float y = Fmadd(2, u.y(), -1);
  bool quadrant = std::abs(x) < std::abs(y);
  Float r = quadrant ? y : x;
  Float rp = quadrant ? x : y;
  Float phi = Float(0.25) * PI * rp / r;
  if (quadrant) {
    phi = Float(0.5) * PI - phi;
  }
  if (x == 0 && y == 0) {
    phi = 0;
  }
  auto [s, c] = SinCos(phi);
  return Vector2(r * c, r * s);
}

Float SquareToUniformDiskConcentricPdf(const Vector2& p) {
  return 1 / PI;
}

}  // namespace Rad::Warp
