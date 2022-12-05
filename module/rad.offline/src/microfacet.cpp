#include <rad/offline/render/microfacet.h>

#include <rad/offline/math_ext.h>
#include <rad/offline/frame.h>

//#define RAD_USE_ORIGINAL_MICROFACEET  //定义这个宏是使用原始公式, 不定义就使用mitsuba优化的代码

using namespace Rad::Math;

namespace Rad {

Float Beckmann::DImpl(const Vector3& wh) const {
#if defined(RAD_USE_ORIGINAL_MICROFACEET)
  Float tan2Theta = Frame::Tan2Theta(wh);
  if (std::isinf(tan2Theta)) {  //掠射
    return 0;
  }
  Float cos4Theta = Sqr(Frame::Cos2Theta(wh));
  return std::exp(-tan2Theta * (Frame::Cos2Phi(wh) / Sqr(alphaX) + Frame::Sin2Phi(wh) / Sqr(alphaY))) / (PI * alphaX * alphaY * cos4Theta);
#else
  // mitsuba优化的代码
  Float alphaXY = alphaX * alphaY;
  Float cosTheta = Frame::CosTheta(wh);
  Float cosTheta2 = Sqr(cosTheta);
  Float result = std::exp(-(Sqr(wh.x() / alphaX) + Sqr(wh.y() / alphaY)) / cosTheta2) / (PI * alphaXY * Sqr(cosTheta2));
  return result * cosTheta > 1e-20f ? result : 0;
#endif
}

#if defined(RAD_USE_ORIGINAL_MICROFACEET)
static Float LambdaBeckmann(const Vector3& w, Float alphaX, Float alphaY) {
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
#endif

Float Beckmann::SmithG1Impl(const Vector3& v, const Vector3& wh) const {
#if defined(RAD_USE_ORIGINAL_MICROFACEET)
  Float result = 1 / (1 + LambdaBeckmann(v, alphaX, alphaY));
  if (v.dot(wh) * Frame::CosTheta(v) <= 0.f) {
    result = 0;
  }
  return result;
#else
  Float xyAlpha2 = Sqr(alphaX * v.x()) + Sqr(alphaY * v.y());
  Float tanThetaAlpha2 = xyAlpha2 / Sqr(v.z());
  Float a = Rsqrt(tanThetaAlpha2);
  Float aSqr = Sqr(a);
  Float result = a >= 1.6f ? 1.f : (3.535f * a + 2.181f * aSqr) / (1.f + 2.276f * a + 2.577f * aSqr);
  if (xyAlpha2 == 0) {
    result = 1;
  }
  if (v.dot(wh) * Frame::CosTheta(v) <= 0.f) {
    result = 0;
  }
  return result;
#endif
}

Float Beckmann::GImpl(const Vector3& wi, const Vector3& wo, const Vector3& wh) const {
  return SmithG1Impl(wi, wh) * SmithG1Impl(wo, wh);  // Separable Masking and Shadowing Function
}

Float Beckmann::PdfImpl(const Vector3& wi, const Vector3& wh) const {
  Float result = DImpl(wh);
  if (sampleVisible) {
    result *= SmithG1Impl(wi, wh) * AbsDot(wi, wh) / Frame::CosTheta(wi);
  } else {
    result *= Frame::CosTheta(wh);
  }
  return result;
}

static Vector2 SampleVisibleBeckmann(Float cosThetaI, Vector2 sample) {
  // mitsuba impl
  Float tanThetaI = SafeSqrt(Fmadd(-cosThetaI, cosThetaI, Float(1))) / cosThetaI;
  Float cotThetaI = Rcp(tanThetaI);
  Float maxval = std::erf(cotThetaI);
  sample = sample.cwiseMin(Float(1 - 1e-6)).cwiseMax(Float(1e-6));
  Float x = maxval - (maxval + 1) * std::erf(std::sqrt(-std::log(sample.x())));
  sample.x() *= 1 + maxval + (1 / std::sqrt(PI)) * tanThetaI * std::exp(-Sqr(cotThetaI));
  for (size_t i = 0; i < 3; i++) {
    Float slope = ErfInv(x);
    Float value = 1 + x + (1 / std::sqrt(PI)) * tanThetaI * std::exp(-Sqr(slope)) - sample.x();
    Float derivative = 1 - slope * tanThetaI;
    x -= value / derivative;
  }
  return Vector2(ErfInv(x), ErfInv(Fmadd(Float(2), sample.y(), Float(-1))));
}

std::pair<Vector3, Float> Beckmann::SampleImpl(const Vector3& wi, const Vector2& xi) const {
  if (sampleVisible) {
    Vector3 wiP = Vector3(alphaX * wi.x(), alphaY * wi.y(), wi.z()).normalized();
    Float sinPhi, cosPhi;
    std::tie(sinPhi, cosPhi) = Frame::SinCosPhi(wiP);
    Float cosTheta = Frame::CosTheta(wiP);
    Vector2 slope = SampleVisibleBeckmann(cosTheta, xi);
    slope = Vector2(
        Fmadd(cosPhi, slope.x(), -(sinPhi * slope.y())) * alphaX,
        Fmadd(sinPhi, slope.x(), cosPhi * slope.y()) * alphaY);
    Vector3 m = Vector3(-slope.x(), -slope.y(), 1).normalized();
    Float pdf = DImpl(m) * SmithG1Impl(wi, m) * AbsDot(wi, m) / Frame::CosTheta(wi);
    return {m, pdf};
  } else {
    Float sinPhi, cosPhi, alpha2;
    if (alphaX == alphaY) {
      std::tie(sinPhi, cosPhi) = SinCos((2 * PI) * xi.y());
      alpha2 = alphaX * alphaY;
    } else {
      Float ratio = alphaY / alphaX;
      Float tmp = ratio * std::tan((2 * PI) * xi.y());
      cosPhi = Rsqrt(Fmadd(tmp, tmp, 1));
      cosPhi = MulSign(cosPhi, std::abs(xi.y() - Float(0.5)) - Float(0.25));
      sinPhi = cosPhi * tmp;
      alpha2 = Rcp(Sqr(cosPhi / alphaX) + Sqr(sinPhi / alphaY));
    }
    Float cosTheta = Rsqrt(Fmadd(-alpha2, std::log(1 - xi.x()), 1));
    Float cos2Theta = Sqr(cosTheta);
    Float cos3Theta = std::max(cos2Theta * cosTheta, Float(1e-20));
    Float pdf = (1.f - xi.x()) / (PI * alphaX * alphaY * cos3Theta);
    Float sinTheta = std::sqrt(1 - cos2Theta);
    Vector3 m(
        cosPhi * sinTheta,
        sinPhi * sinTheta,
        cosTheta);
    return {m, pdf};
  }
}

Float GGX::DImpl(const Vector3& wh) const {
#if defined(RAD_USE_ORIGINAL_MICROFACEET)
  Float tan2Theta = Frame::Tan2Theta(wh);
  if (std::isinf(tan2Theta)) {
    return 0.0f;
  }  //掠射
  Float cos4Theta = Sqr(Frame::Cos2Theta(wh));
  Float e = (Frame::Cos2Phi(wh) / Sqr(alphaX) + Frame::Sin2Phi(wh) / Sqr(alphaY)) * tan2Theta;
  return 1 / (PI * alphaX * alphaY * cos4Theta * Sqr(1 + e));
#else
  Float alphaXY = alphaX * alphaY;
  Float cosTheta = Frame::CosTheta(wh);
  Float result = Rcp(PI * alphaXY * Sqr(Sqr(wh.x() / alphaX) + Sqr(wh.y() / alphaY) + Sqr(wh.z())));
  return result * cosTheta > 1e-20f ? result : 0;
#endif
}

#if defined(RAD_USE_ORIGINAL_MICROFACEET)
static Float LambdaGGX(const Vector3& w, Float alphaX, Float alphaY) {
  Float absTanTheta = std::abs(Frame::TanTheta(w));
  if (std::isinf(absTanTheta)) {
    return 0.0f;
  }
  Float alpha = std::sqrt(Frame::Cos2Phi(w) * Sqr(alphaX) + Frame::Sin2Phi(w) * Sqr(alphaY));
  Float alpha2Tan2Theta = Sqr(alpha * absTanTheta);
  return (-1 + std::sqrt(1.0f + alpha2Tan2Theta)) / 2;
}
#endif

Float GGX::SmithG1Impl(const Vector3& v, const Vector3& wh) const {
#if defined(RAD_USE_ORIGINAL_MICROFACEET)
  Float result = 1 / (1 + LambdaGGX(v, alphaX, alphaY));
  if (v.dot(wh) * Frame::CosTheta(v) <= 0.f) {
    result = 0;
  }
  return result;
#else
  Float xyAlpha2 = Sqr(alphaX * v.x()) + Sqr(alphaY * v.y());
  Float tanThetaAlpha2 = xyAlpha2 / Sqr(v.z());
  Float result = 2 / (1 + std::sqrt(1 + tanThetaAlpha2));
  if (xyAlpha2 == 0) {
    result = 1;
  }
  if (v.dot(wh) * Frame::CosTheta(v) <= 0) {
    result = 0;
  }
  return result;
#endif
}

Float GGX::GImpl(const Vector3& wi, const Vector3& wo, const Vector3& wh) const {
  return SmithG1Impl(wi, wh) * SmithG1Impl(wo, wh);  // Separable Masking and Shadowing Function
}

Float GGX::PdfImpl(const Vector3& wi, const Vector3& wh) const {
  Float result = DImpl(wh);
  if (sampleVisible) {
    result *= SmithG1Impl(wi, wh) * AbsDot(wi, wh) / Frame::CosTheta(wi);
  } else {
    result *= Frame::CosTheta(wh);
  }
  return result;
}

static Vector2 SampleVisibleGGX(Float cosThetaI, Vector2 sample) {
  // PBRT impl
  if (cosThetaI > 0.9999) {
    Float r = sqrt(sample.x() / (1 - sample.x()));
    Float phi = Float(6.28318530718) * sample.y();
    return Vector2(r * cos(phi), r * sin(phi));
  }
  Float sinTheta = std::sqrt(std::max((Float)0, (Float)1 - Sqr(cosThetaI)));
  Float tanTheta = sinTheta / cosThetaI;
  Float a = 1 / tanTheta;
  Float G1 = 2 / (1 + std::sqrt(1 + 1 / Sqr(a)));
  Float A = 2 * sample.x() / G1 - 1;
  Float tmp = 1.f / Fmadd(A, A, -1);
  if (tmp > 1e10) {
    tmp = Float(1e10);
  }
  Float B = tanTheta;
  Float D = std::sqrt(std::max(Float(Sqr(B) * Sqr(tmp) - (Sqr(A) - Sqr(B)) * tmp), Float(0)));
  Float slopeX1 = Fmadd(B, tmp, -D);
  Float slopeX2 = Fmadd(B, tmp, D);
  Float slopeX = (A < 0 || slopeX2 > 1 / tanTheta) ? slopeX1 : slopeX2;
  Float S;
  if (sample.y() > 0.5f) {
    S = 1;
    sample.y() = 2 * (sample.y() - Float(0.5));
  } else {
    S = -1;
    sample.y() = 2 * (Float(0.5) - sample.y());
  }
  Float z =
      (sample.y() * (sample.y() * (sample.y() * 0.27385f - 0.73369f) + 0.46341f)) /
      (sample.y() * (sample.y() * (sample.y() * 0.093073f + 0.309420f) - 1) + 0.597999f);
  Float slopeY = S * z * std::sqrt(1 + slopeX * slopeX);
  return Vector2(slopeX, slopeY);
}

std::pair<Vector3, Float> GGX::SampleImpl(const Vector3& wi, const Vector2& xi) const {
  if (sampleVisible) {
    Vector3 wiP = Vector3(alphaX * wi.x(), alphaY * wi.y(), wi.z()).normalized();
    Float sinPhi, cosPhi;
    std::tie(sinPhi, cosPhi) = Frame::SinCosPhi(wiP);
    Float cosTheta = Frame::CosTheta(wiP);
    Vector2 slope = SampleVisibleGGX(cosTheta, xi);
    slope = Vector2(
        Fmadd(cosPhi, slope.x(), -(sinPhi * slope.y())) * alphaX,
        Fmadd(sinPhi, slope.x(), cosPhi * slope.y()) * alphaY);
    Vector3 m = Vector3(-slope.x(), -slope.y(), 1).normalized();
    Float pdf = DImpl(m) * SmithG1Impl(wi, m) * AbsDot(wi, m) / Frame::CosTheta(wi);
    return {m, pdf};
  } else {
    Float sinPhi, cosPhi, alpha2;
    if (alphaX == alphaY) {
      std::tie(sinPhi, cosPhi) = SinCos((2 * PI) * xi.y());
      alpha2 = alphaX * alphaY;
    } else {
      Float ratio = alphaY / alphaX;
      Float tmp = ratio * std::tan((2 * PI) * xi.y());
      cosPhi = Rsqrt(Fmadd(tmp, tmp, 1));
      cosPhi = MulSign(cosPhi, std::abs(xi.y() - Float(0.5)) - Float(0.25));
      sinPhi = cosPhi * tmp;
      alpha2 = Rcp(Sqr(cosPhi / alphaX) + Sqr(sinPhi / alphaY));
    }
    Float tanThetaM2 = alpha2 * xi.x() / (1.f - xi.x());
    Float cosTheta = Rsqrt(1.f + tanThetaM2);
    Float cos2Theta = Sqr(cosTheta);
    Float temp = 1 + tanThetaM2 / alpha2;
    Float cos3Theta = std::max(cos2Theta * cosTheta, Float(1e-20));
    Float pdf = Rcp(PI * alphaX * alphaY * cos3Theta * Sqr(temp));
    Float sinTheta = std::sqrt(1 - cos2Theta);
    Vector3 m(
        cosPhi * sinTheta,
        sinPhi * sinTheta,
        cosTheta);
    return {m, pdf};
  }
}

bool Beckmann::IsSampleVisibleImpl() const {
  return sampleVisible;
}

bool GGX::IsSampleVisibleImpl() const {
  return sampleVisible;
}

}  // namespace Rad
