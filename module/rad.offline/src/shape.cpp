#include <rad/offline/render/shape.h>

#include <rad/offline/math_ext.h>

namespace Rad {

DirectionSampleResult Shape::SampleDirection(const Interaction& ref, const Vector2& xi) const {
  PositionSampleResult psr = SamplePosition(xi);
  DirectionSampleResult dsr{psr};
  dsr.Dir = dsr.P - ref.P;
  Float distanceSquared = dsr.Dir.squaredNorm();
  dsr.Dist = std::sqrt(distanceSquared);
  dsr.Dir /= dsr.Dist;
  Float dp = std::abs(dsr.Dir.dot(dsr.N));
  Float x = distanceSquared / dp;
  dsr.Pdf *= std::isfinite(x) ? x : 0;
  return dsr;
}

Float Shape::PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const {
  Float pdf = PdfPosition(dsr);
  Float dp = std::abs(dsr.Dir.dot(dsr.N));
  pdf *= dp == 0 ? 0 : Math::Sqr(dsr.Dist) / dp;
  return pdf;
}

SurfaceInteraction Shape::EvalParamSurface(const Vector2& uv) {
  throw RadNotSupportedException("no impl EvalParamSurface()");
}

}  // namespace Rad
