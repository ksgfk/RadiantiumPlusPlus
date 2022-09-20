#include <rad/offline/render/microfacet.h>

using namespace Rad::Math;

namespace Rad {

Float Microfacet::Beckmann::D(const Vector3& wh, Float alphaX, Float alphaY) {
  Float tan2Theta = Frame::Tan2Theta(wh);
  if (std::isinf(tan2Theta)) {  //掠射
    return 0;
  }
  Float cos4Theta = Sqr(Frame::Cos2Theta(wh));
  return std::exp(-tan2Theta * (Frame::Cos2Phi(wh) / Sqr(alphaX) + Frame::Sin2Phi(wh) / Sqr(alphaY))) / (PI * alphaX * alphaY * cos4Theta);
}

Float Microfacet::Beckmann::Lambda(const Vector3& w, Float alphaX, Float alphaY) {
  Float absTanTheta = std::abs(Frame::TanTheta(w));
  if (std::isinf(absTanTheta)) {
    return 0;
  }
  Float alpha = std::sqrt(Frame::Cos2Phi(w) * Sqr(alphaX) + Frame::Sin2Phi(w) * Sqr(alphaY));
  Float a = 1 / (alpha * absTanTheta);
  if (a >= Float(1.6)) {
    return 0;
  }
  return (1 - Float(1.259) * a + Float(0.396) * a * a) / (Float(3.535) * a + Float(2.181) * a * a);
}

Float Microfacet::Beckmann::G(const Vector3& wi, const Vector3& wo, Float alphaX, Float alphaY) {
  return 1 / (1 + Lambda(wo, alphaX, alphaY) + Lambda(wi, alphaX, alphaY));
}

Float Microfacet::Beckmann::Pdf(const Vector3& wi, const Vector3& wh, Float alphaX, Float alphaY) {
  Float d = D(wh, alphaX, alphaY);
  Float cosTheta = Frame::AbsCosTheta(wh);
  return d * cosTheta;
}

Vector3 Microfacet::Beckmann::Sample(const Vector3& wi, Float alphaX, Float alphaY, const Vector2 xi) {
  Float tan2Theta;
  Vector2 u = xi;
  Float cosPhi, sinPhi;
  if (alphaX == alphaY) {
    Float logSample = std::log(1 - u.x());
    tan2Theta = -Sqr(alphaX) * logSample;
    Float phi = u.y() * 2 * PI;
    std::tie(sinPhi, cosPhi) = SinCos(phi);
  } else {
    Float logSample = std::log(1 - u.x());
    Float phi = std::atan(alphaY / alphaX * std::tan(2 * PI * u.y() + 0.5f * PI));
    if (u.y() > 0.5f) {
      phi += PI;
    }
    std::tie(sinPhi, cosPhi) = SinCos(phi);
    Float alphaX2 = Sqr(alphaX), alphaY2 = Sqr(alphaY);
    tan2Theta = -logSample / (Sqr(cosPhi) / alphaX2 + Sqr(sinPhi) / alphaY2);
  }
  Float cosTheta = 1 / std::sqrt(1 + tan2Theta);
  Float sinTheta = SafeSqrt(1 - Sqr(cosTheta));
  Vector3 wh(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
  if (!Frame::IsSameHemisphere(wi, wh)) {
    wh = -wh;
  }
  return wh;
}

Float Microfacet::GGX::D(const Vector3& wh, Float alphaX, Float alphaY) {
  Float tan2Theta = Frame::Tan2Theta(wh);
  if (std::isinf(tan2Theta)) {
    return 0.0f;
  }  //掠射
  Float cos4Theta = Sqr(Frame::Cos2Theta(wh));
  Float e = (Frame::Cos2Phi(wh) / Sqr(alphaX) + Frame::Sin2Phi(wh) / Sqr(alphaY)) * tan2Theta;
  return 1 / (PI * alphaX * alphaY * cos4Theta * Sqr(1 + e));
}

Float Microfacet::GGX::Lambda(const Vector3& w, Float alphaX, Float alphaY) {
  Float absTanTheta = std::abs(Frame::TanTheta(w));
  if (std::isinf(absTanTheta)) {
    return 0.0f;
  }
  Float alpha = std::sqrt(Frame::Cos2Phi(w) * Sqr(alphaX) + Frame::Sin2Phi(w) * Sqr(alphaY));
  Float alpha2Tan2Theta = Sqr(alpha * absTanTheta);
  return (-1 + std::sqrt(1.0f + alpha2Tan2Theta)) / 2;
}

Float Microfacet::GGX::G(const Vector3& wi, const Vector3& wo, Float alphaX, Float alphaY) {
  return 1 / (1 + Lambda(wo, alphaX, alphaY) + Lambda(wi, alphaX, alphaY));
}

Float Microfacet::GGX::Pdf(const Vector3& wi, const Vector3& wh, Float alphaX, Float alphaY) {
  Float d = D(wh, alphaX, alphaY);
  Float cosTheta = Frame::AbsCosTheta(wh);
  return d * cosTheta;
}

Vector3 Microfacet::GGX::Sample(const Vector3& wi, Float alphaX, Float alphaY, const Vector2 xi) {
  Vector2 u = xi;
  Float phi = (2 * PI) * u.y();
  Float cosTheta;
  Float cosPhi, sinPhi;
  if (alphaX == alphaY) {
    Float tanTheta2 = Sqr(alphaX) * u.x() / (1.0f - u.x());
    cosTheta = 1 / std::sqrt(1 + tanTheta2);
    std::tie(sinPhi, cosPhi) = SinCos(phi);
  } else {
    phi = std::atan(alphaY / alphaX * std::tan(2 * PI * u.y() + 0.5f * PI));
    if (u.y() > 0.5f) {
      phi += PI;
    }
    std::tie(sinPhi, cosPhi) = SinCos(phi);
    Float alphaX2 = Sqr(alphaX), alphaY2 = Sqr(alphaY);
    Float alpha2 = 1 / (cosPhi * cosPhi / alphaX2 + sinPhi * sinPhi / alphaY2);
    Float tanTheta2 = alpha2 * u.x() / (1 - u.x());
    cosTheta = 1 / std::sqrt(1 + tanTheta2);
  }
  Float sinTheta = SafeSqrt(1.0f - Sqr(cosTheta));
  Vector3 wh(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
  if (!Frame::IsSameHemisphere(wi, wh)) {
    wh = -wh;
  }
  return wh;
}

}  // namespace Rad
