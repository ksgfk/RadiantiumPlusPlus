#include <rad/offline/render/fresnel.h>

using namespace Rad::Math;

namespace Rad {

Vector3 Fresnel::Reflect(const Vector3& wi) {
  return Vector3(-wi.x(), -wi.y(), wi.z());
}

Vector3 Fresnel::Reflect(const Vector3& wi, const Vector3& wh) {
  return Fmadd(wh, Vector3::Constant(2 * wi.dot(wh)), -wi);
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

}  // namespace Rad
