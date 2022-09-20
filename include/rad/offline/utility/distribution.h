#include "../common.h"

namespace Rad {

class DiscreteDistribution1D {
 public:
  DiscreteDistribution1D() = default;
  DiscreteDistribution1D(const std::vector<Float>& data);
  DiscreteDistribution1D(const Float* data, size_t size);

  Float Sum() const { return _sum; }
  Float Normalization() const { return _normalization; }

  size_t Sample(Float xi) const;
  std::pair<size_t, Float> SampleWithPdf(Float xi) const;
  std::pair<size_t, Float> SampleReuse(Float xi) const;
  Float Pdf(size_t index) const;

 private:
  void Update();

  std::vector<Float> _cdf;
  Float _sum = 0;
  Float _normalization = 0;
};

}  // namespace Rad
