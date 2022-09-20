#include <rad/offline/utility/distribution.h>

#include <algorithm>

namespace Rad {

DiscreteDistribution1D::DiscreteDistribution1D(const Float* data, size_t size) {
  _cdf.resize(size);
  Float64 sum = 0;
  for (size_t i = 0; i < size; i++) {
    sum += data[i];
    _cdf[i] = Float(sum);
  }
  _sum = Float(sum);
  _normalization = Float(1.0 / sum);
}

DiscreteDistribution1D::DiscreteDistribution1D(const std::vector<Float>& data)
    : DiscreteDistribution1D(data.data(), data.size()) {}

size_t DiscreteDistribution1D::Sample(Float xi) const {
  Float value = xi * _sum;
  // lower_bound 返回大于等于value的迭代器, 如果找不到(也就是比最大的还要大), 则返回end()
  auto iter = std::lower_bound(_cdf.begin(), _cdf.end(), value);
  size_t index = (size_t)std::max((ptrdiff_t)0, iter - _cdf.begin());
  return std::min(index, _cdf.size() - 1);
}

std::pair<size_t, Float> DiscreteDistribution1D::SampleWithPdf(Float xi) const {
  size_t index = Sample(xi);
  return std::make_pair(index, Pdf(index));
}

std::pair<size_t, Float> DiscreteDistribution1D::SampleReuse(Float xi) const {
  size_t index = Sample(xi);
  Float cdf = index == 0 ? 0 : _cdf[index - 1] * _normalization;
  Float pdf = Pdf(index);
  Float reuse = (xi - cdf) / pdf;
  return std::make_pair(index, reuse);
}

Float DiscreteDistribution1D::Pdf(size_t index) const {
  if (index == 0) {
    return _cdf[0] * _normalization;
  } else {
    return (_cdf[index] - _cdf[index - 1]) * _normalization;
  }
}

}  // namespace Rad
