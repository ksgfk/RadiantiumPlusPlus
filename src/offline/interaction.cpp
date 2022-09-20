#include <rad/offline/render/interaction.h>

#include <rad/offline/render.h>

namespace Rad {

Vector3 Interaction::OffsetP(const Vector3& d) const {
  Float mag = (1 + P.cwiseAbs().maxCoeff()) * Math::RayEpsilon;
  mag = Math::MulSign(mag, N.dot(d));
  return Math::Fmadd(Vector3(mag, mag, mag), N, P);
}

Ray Interaction::SpawnRay(const Vector3& d) const {
  return Ray{OffsetP(d), d, 0, std::numeric_limits<Float>::max()};
}

Ray Interaction::SpawnRayTo(const Vector3& p) const {
  Vector3 o = OffsetP(p - P);
  Vector3 d = p - o;
  Float dist = d.norm();
  d /= dist;
  return Ray{o, d, 0, dist * (1 - Math::ShadowEpsilon)};
}

bool Interaction::IsValid() const {
  return T != std::numeric_limits<Float>::infinity();
}

SurfaceInteraction::SurfaceInteraction(const PositionSampleResult& psr) {
  P = psr.P;
  N = psr.N;
  UV = psr.UV;
  Shading = Frame(N);

  T = 0;
  Shape = nullptr;
  ShapeIndex = std::numeric_limits<UInt32>::max();
  dPdU = dPdV = dNdU = dNdV = Wi = Vector3::Zero();
}

void SurfaceInteraction::ComputeShadingFrame() {
  if (dPdU.isZero()) {
    Shading.S = Math::CoordinateSystem(Shading.N).first;
  } else {
    Shading.S = (Shading.N * (-Shading.N.dot(dPdU)) + dPdU).normalized();
  }
  Shading.T = Shading.N.cross(Shading.S);
}

Vector3 SurfaceInteraction::ToWorld(const Vector3& v) const { return Shading.ToWorld(v); }

Vector3 SurfaceInteraction::ToLocal(const Vector3& v) const { return Shading.ToLocal(v); }

PositionSampleResult SurfaceInteraction::ToPsr() const {
  PositionSampleResult psr{};
  psr.P = P;
  psr.N = Shading.N;
  psr.UV = UV;
  psr.Pdf = 0;
  psr.IsDelta = false;
  return psr;
}

DirectionSampleResult SurfaceInteraction::ToDsr(const Interaction& ref) const {
  DirectionSampleResult dsr{ToPsr()};
  Vector3 rel = P - ref.P;
  dsr.Dist = rel.norm();
  dsr.Dir = this->IsValid() ? Vector3(rel / dsr.Dist) : -Wi;
  return dsr;
}

SurfaceInteraction HitShapeRecord::ComputeSurfaceInteraction(const Ray& ray) const {
  SurfaceInteraction si = ShapePtr->ComputeInteraction(ray, *this);
  si.ComputeShadingFrame();
  si.Wi = si.ToLocal(-ray.D);
  si.ShapeIndex = ShapeIndex;
  return si;
}

}  // namespace Rad
