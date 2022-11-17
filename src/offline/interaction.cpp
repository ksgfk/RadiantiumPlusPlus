#include <rad/offline/render/interaction.h>

#include <rad/offline/render.h>

using namespace Rad::Math;

namespace Rad {

Vector3 Interaction::OffsetP(const Vector3& d) const {
  Float mag = (1 + P.cwiseAbs().maxCoeff()) * Math::RayEpsilon;
  mag = Math::MulSign(mag, N.dot(d));
  return Math::Fmadd(Vector3::Constant(mag), N, P);
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
  dUVdX = dUVdY = Vector2::Zero();
}

void SurfaceInteraction::ComputeShadingFrame() {
  // Gram-schmidt 正交化求正交基
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

Bsdf* SurfaceInteraction::BSDF() {
  dUVdX = Vector2(0, 0);
  dUVdY = Vector2(0, 0);
  return Shape->GetBsdf();
}

Bsdf* SurfaceInteraction::BSDF(const RayDifferential& ray) {
  // https://www.pbr-book.org/3ed-2018/Texture/Sampling_and_Antialiasing#FindingtheTextureSamplingRate
  if (ray.HasDifferentials && dUVdX.isZero() && dUVdY.isZero()) {
    //首先计算与法线切平面的两个交点距离, 射线与平面求交的推导在 shape/rectangle.cpp 里面
    Float d = N.dot(P);
    Float tx = (d - N.dot(ray.Ox)) / N.dot(ray.Dx);
    Float ty = (d - N.dot(ray.Oy)) / N.dot(ray.Dy);
    //计算坐标的变化
    Vector3 dpdx = (ray.Dx * tx + ray.Ox) - P;
    Vector3 dpdy = (ray.Dy * ty + ray.Oy) - P;
    /*
     * 看图: https://www.pbr-book.org/3ed-2018/Texture/Sampling_and_Antialiasing#fig:point-wrt-tangent-coordsys
     * 解方程, 这有三个等式与两个未知数
     * pbrt怎么解的看不懂, 先抄一下mitsuba的实现
     */
    Float a00 = dPdU.dot(dPdU),
          a01 = dPdU.dot(dPdV),
          a11 = dPdV.dot(dPdV),
          invDet = Rcp(Fmadd(a00, a11, -(a01 * a01)));
    Float b0x = dPdU.dot(dpdx),
          b1x = dPdV.dot(dpdx),
          b0y = dPdU.dot(dpdy),
          b1y = dPdV.dot(dpdy);
    invDet = std::isfinite(invDet) ? invDet : 0;
    dUVdX = Vector2(Fmadd(a11, b0x, -(a01 * b1x)), Fmadd(a00, b1x, -(a01 * b0x))) * invDet;
    dUVdY = Vector2(Fmadd(a11, b0y, -(a01 * b1y)), Fmadd(a00, b1y, -(a01 * b0y))) * invDet;
  }
  return Shape->GetBsdf();
}

const Medium* SurfaceInteraction::GetMedium(const Ray& ray) const {
  return ray.D.dot(N) > 0 ? Shape->GetOutsideMedium() : Shape->GetInsideMedium();
}

Vector3 MediumInteraction::ToWorld(const Vector3& v) const { return Shading.ToWorld(v); }

Vector3 MediumInteraction::ToLocal(const Vector3& v) const { return Shading.ToLocal(v); }

SurfaceInteraction HitShapeRecord::ComputeSurfaceInteraction(const Ray& ray) const {
  SurfaceInteraction si = ShapePtr->ComputeInteraction(ray, *this);
  si.ComputeShadingFrame();
  si.Wi = si.ToLocal(-ray.D);
  si.ShapeIndex = ShapeIndex;
  return si;
}

}  // namespace Rad
