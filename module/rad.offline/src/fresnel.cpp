#include <rad/offline/render/fresnel.h>

#include <rad/offline/math_ext.h>

using namespace Rad::Math;

namespace Rad {

Vector3 Fresnel::Reflect(const Vector3& wi) {
  return Vector3(-wi.x(), -wi.y(), wi.z());
}

Vector3 Fresnel::Reflect(const Vector3& wi, const Vector3& wh) {
  return Fmadd(wh, Vector3::Constant(2 * wi.dot(wh)), -wi);
}

Vector3 Fresnel::Refract(const Vector3& wi, Float cosThetaT, Float etaTI) {
  return Vector3(-etaTI * wi.x(), -etaTI * wi.y(), cosThetaT);
}

Vector3 Fresnel::Refract(const Vector3& wi, const Vector3& wh, Float cosThetaT, Float etaTI) {
  return Fmadd(wh, Vector3::Constant(Fmadd(wi.dot(wh), etaTI, cosThetaT)), -(wi * etaTI));
}

std::optional<Vector3> Fresnel::Refract(const Vector3& wi, const Vector3& wh, Float eta) {
  Float cosThetaI = wh.dot(wi);
  Float sin2ThetaI = std::max(Float(0), 1 - Sqr(cosThetaI * cosThetaI));
  Float sin2ThetaT = eta * eta * sin2ThetaI;
  if (sin2ThetaT >= 1) {
    return std::nullopt;
  }
  Float cosThetaT = std::sqrt(1 - sin2ThetaT);
  Vector3 wo = (eta * -wi + (eta * cosThetaI - cosThetaT) * wh).normalized();
  return std::make_optional(wo);
}

std::tuple<Float, Float, Float, Float> Fresnel::Dielectric(Float cosThetaI, Float eta) {
  auto isOutside = cosThetaI >= 0;
  Float rcpEta = Rcp(eta);
  Float etaIT = isOutside ? eta : rcpEta;
  Float etaTI = isOutside ? rcpEta : eta;
  Float sqrCosThetaT = Fmadd(-Fmadd(-cosThetaI, cosThetaI, 1), etaTI * etaTI, 1);
  Float absCosThetaI = std::abs(cosThetaI);
  Float absCosThetaT = SafeSqrt(sqrCosThetaT);
  auto indexMatched = eta == 1;
  auto specialCase = indexMatched || absCosThetaI == 0;
  Float rSC = (indexMatched ? Float(0.f) : Float(1.f));
  Float aS = Fmadd(-etaIT, absCosThetaT, absCosThetaI) /
             Fmadd(etaIT, absCosThetaT, absCosThetaI);
  Float aP = Fmadd(-etaIT, absCosThetaI, absCosThetaT) /
             Fmadd(etaIT, absCosThetaI, absCosThetaT);
  Float r = Float(0.5) * (Sqr(aS) + Sqr(aP));
  if (specialCase) {
    r = rSC;
  }
  Float cosThetaT = cosThetaI >= 0 ? -absCosThetaT : absCosThetaT;
  return std::make_tuple(r, cosThetaT, etaIT, etaTI);
}

Vector3 Fresnel::Conductor(Float cosThetaI, const Vector3& eta, const Vector3& k) {
  Float cosThetaI2 = cosThetaI * cosThetaI,
        sinThetaI2 = Float(1) - cosThetaI2,
        sinThetaI4 = sinThetaI2 * sinThetaI2;
  auto etaR = eta,
       etaI = k;
  auto temp1 = etaR.cwiseProduct(etaR) - etaI.cwiseProduct(etaI) - Vector3::Constant(sinThetaI2);
  auto a2pb2 = (temp1.cwiseProduct(temp1) + etaI.cwiseProduct(etaI).cwiseProduct(etaR).cwiseProduct(etaR) * 4).cwiseMax(0).cwiseSqrt();
  auto a = ((a2pb2 + temp1) * Float(0.5)).cwiseMax(0).cwiseSqrt();
  auto term1 = a2pb2 + Vector3::Constant(cosThetaI2);
  auto term2 = a * 2 * cosThetaI;
  auto rs = (term1 - term2).cwiseProduct((term1 + term2).cwiseInverse());
  auto term3 = a2pb2 * cosThetaI2 + Vector3::Constant(sinThetaI4);
  auto term4 = term2 * sinThetaI2;
  auto rp = rs.cwiseProduct(term3 - term4).cwiseProduct((term3 + term4).cwiseInverse());
  return (rs + rp) * Float(0.5);
}

Float Fresnel::DiffuseReflectance(Float eta) {
  Float invEta = Rcp(eta);
  return eta < 1
             ? Fmadd(Float(0.0636f), invEta, Fmadd(eta, Fmadd(eta, Float(-1.4399f), Float(0.7099f)), Float(0.6681f)))
             : Horner(invEta, Float(0.919317f), Float(-3.4793f), Float(6.75335f), Float(-7.80989f), Float(4.98554f), Float(-1.36881f));
}

}  // namespace Rad
