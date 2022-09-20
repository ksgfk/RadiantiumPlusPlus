#include "../common.h"

#include <type_traits>

namespace Rad {

enum class MicrofacetDistributionType {
  Beckmann,
  GGX
};

class Microfacet {
 public:
  class Beckmann {
   public:
    static Float D(const Vector3& wh, Float alphaX, Float alphaY);
    static Float Lambda(const Vector3& w, Float alphaX, Float alphaY);
    static Float G(const Vector3& wi, const Vector3& wo, float alphaX, float alphaY);
    static Float Pdf(const Vector3& wi, const Vector3& wh, float alphaX, float alphaY);
    static Vector3 Sample(const Vector3& wi, float alphaX, float alphaY, const Vector2 xi);
  };

  class GGX {
   public:
    static Float D(const Vector3& wh, Float alphaX, Float alphaY);
    static Float Lambda(const Vector3& w, Float alphaX, Float alphaY);
    static Float G(const Vector3& wi, const Vector3& wo, float alphaX, float alphaY);
    static Float Pdf(const Vector3& wi, const Vector3& wh, float alphaX, float alphaY);
    static Vector3 Sample(const Vector3& wi, float alphaX, float alphaY, const Vector2 xi);
  };
};

template <typename T>
class MicrofacetDistribution {
 public:
  MicrofacetDistribution(Float alphaX, Float alphaY) : alphaX(alphaX), alphaY(alphaY) {}
  virtual ~MicrofacetDistribution() noexcept = default;

  Float D(const Vector3& wh) const {
    return static_cast<const T*>(this)->DImpl(wh);
  }
  Float G(const Vector3& wi, const Vector3& wo) const {
    return static_cast<const T*>(this)->GImpl(wi, wo);
  }
  Float Pdf(const Vector3& wi, const Vector3& wh) const {
    return static_cast<const T*>(this)->PdfImpl(wi, wh);
  }
  Vector3 Sample(const Vector3& wi, const Vector2 xi) const {
    return static_cast<const T*>(this)->SampleImpl(wi, xi);
  }

  Float alphaX, alphaY;
};

class Beckmann final : public MicrofacetDistribution<Beckmann> {
 public:
  inline Beckmann(Float alphaX, Float alphaY) : MicrofacetDistribution<Beckmann>(alphaX, alphaY) {}
  ~Beckmann() noexcept override = default;

  Float DImpl(const Vector3& wh) const {
    return Microfacet::Beckmann::D(wh, this->alphaX, this->alphaY);
  }
  Float GImpl(const Vector3& wi, const Vector3& wo) const {
    return Microfacet::Beckmann::G(wi, wo, this->alphaX, this->alphaY);
  }
  Float PdfImpl(const Vector3& wi, const Vector3& wh) const {
    return Microfacet::Beckmann::Pdf(wi, wh, this->alphaX, this->alphaY);
  }
  Vector3 SampleImpl(const Vector3& wi, const Vector2 xi) const {
    return Microfacet::Beckmann::Sample(wi, this->alphaX, this->alphaY, xi);
  }
};

class GGX final : public MicrofacetDistribution<GGX> {
 public:
  inline GGX(Float alphaX, Float alphaY) : MicrofacetDistribution<GGX>(alphaX, alphaY) {}
  ~GGX() noexcept override = default;

  Float DImpl(const Vector3& wh) const {
    return Microfacet::GGX::D(wh, this->alphaX, this->alphaY);
  }
  Float GImpl(const Vector3& wi, const Vector3& wo) const {
    return Microfacet::GGX::G(wi, wo, this->alphaX, this->alphaY);
  }
  Float PdfImpl(const Vector3& wi, const Vector3& wh) const {
    return Microfacet::GGX::Pdf(wi, wh, this->alphaX, this->alphaY);
  }
  Vector3 SampleImpl(const Vector3& wi, const Vector2 xi) const {
    return Microfacet::GGX::Sample(wi, this->alphaX, this->alphaY, xi);
  }
};

}  // namespace Rad
