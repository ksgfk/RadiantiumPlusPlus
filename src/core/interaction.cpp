#include <radiantium/interaction.h>

#include <radiantium/shape.h>
#include <radiantium/math_ext.h>

namespace rad {

void SurfaceInteraction::ComputeShadingFrame() {
  if (dPdU.isZero()) {
    Shading.S = math::CoordinateSystem(Shading.N).first;
  } else {
    Shading.S = (Shading.N * (-Shading.N.dot(dPdU)) + dPdU).normalized();
  }
  Shading.T = Shading.N.cross(Shading.S);
}

Vec3 SurfaceInteraction::ToWorld(const Vec3& v) const { return Shading.ToWorld(v); }

Vec3 SurfaceInteraction::ToLocal(const Vec3& v) const { return Shading.ToLocal(v); }

SurfaceInteraction HitShapeRecord::ComputeSurfaceInteraction(const Ray& ray) const {
  SurfaceInteraction si = Shape->ComputeInteraction(ray, *this);
  si.ComputeShadingFrame();
  si.Wi = si.ToLocal(-ray.D);
  return si;
}

void ComputeShadingFrame() {
}

}  // namespace rad