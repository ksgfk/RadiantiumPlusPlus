#include <radiantium/interaction.h>

#include <radiantium/shape.h>
#include <radiantium/math_ext.h>
#include <radiantium/world.h>

namespace rad {
Vec3 Interaction::OffsetP(const Vec3& d) const {
  Float mag = (1 + P.cwiseAbs().maxCoeff()) * math::RayEpsilon;
  mag = math::MulSign(mag, N.dot(d));
  return math::Fmadd(Vec3(mag, mag, mag), N, P);
}

Ray Interaction::SpawnRay(const Vec3& d) const {
  return Ray{OffsetP(d), d, 0, std::numeric_limits<Float>::max()};
}

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

SurfaceInteraction HitShapeRecord::ComputeSurfaceInteraction(const Ray& ray, const World* world) const {
  const auto& e = world->GetEntity(EntityIndex);
  auto shape = e.Shape();
  SurfaceInteraction si = shape->ComputeInteraction(ray, *this);
  si.ComputeShadingFrame();
  si.Wi = si.ToLocal(-ray.D);
  return si;
}

}  // namespace rad
