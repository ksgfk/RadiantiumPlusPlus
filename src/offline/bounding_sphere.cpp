#include <rad/offline/bounding_sphere.h>

#include <rad/offline/math_ext.h>

namespace Rad {

void BoundingSphere::Expand(const Vector3& p) {
  Radius = std::max(Radius, (p - Center).norm());
}

bool BoundingSphere::Contains(const Vector3& p) const {
  return (p - Center).squaredNorm() <= Math::Sqr(Radius);
}

BoundingSphere BoundingSphere::FromBox(const BoundingBox3& box) {
  Vector3 c = box.center();
  return {c, (c - box.max()).norm()};
}

}  // namespace Rad
