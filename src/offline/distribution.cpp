#include <rad/offline/utility/distribution.h>

#include <algorithm>

namespace Rad {

Float DiscreteDistribution1D::PmfNormalized(size_t index) const {
  return _pmf[index] * _normalization;
}

Float DiscreteDistribution1D::CdfNormalized(size_t index) const {
  return _cdf[index] * _normalization;
}

DiscreteDistribution1D::DiscreteDistribution1D(const Float* pmf, size_t size) {
  _pmf = std::vector<Float>(pmf, pmf + size);
  _cdf.resize(size);
  Float64 sum = 0;
  for (size_t i = 0; i < size; i++) {
    double value = (double)*pmf++;
    sum += value;
    _cdf[i] = (Float)sum;
  }
  _sum = Float(sum);
  _normalization = Float(1.0 / sum);
}

DiscreteDistribution1D::DiscreteDistribution1D(const std::vector<Float>& data)
    : DiscreteDistribution1D(data.data(), data.size()) {}

size_t DiscreteDistribution1D::Sample(Float xi) const {
  Float value = xi * _sum;
  // lower_bound 返回大于等于value的迭代器(理论上不可能取到等于), 如果找不到(也就是比最大的还要大), 则返回end()
  // xi的范围是[0,1), 乘以cdf最大值绝对不可能比cdf最大值还大, 所以不用处理边界
  auto iter = std::lower_bound(_cdf.begin(), _cdf.end(), value);
  return std::distance(_cdf.begin(), iter);
}

std::pair<size_t, Float> DiscreteDistribution1D::SampleWithPdf(Float xi) const {
  size_t index = Sample(xi);
  return std::make_pair(index, PmfNormalized(index));
}

std::pair<size_t, Float> DiscreteDistribution1D::SampleReuse(Float xi) const {
  size_t index = Sample(xi);
  Float pmf = PmfNormalized(index);
  Float cdf = index == 0 ? 0 : CdfNormalized(index - 1);
  return {index, (xi - cdf) / pmf};
}

}  // namespace Rad
