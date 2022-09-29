#include <rad/offline/utility/distribution.h>

#include <algorithm>

namespace Rad {

Float DiscreteDistribution1D::PmfNormalized(size_t index) const {
  return _pmf[index] * _normalization;
}

Float DiscreteDistribution1D::CdfNormalized(size_t index) const {
  return _cdf[index] * _normalization;
}

size_t DiscreteDistribution1D::Count() const {
  return _pmf.size();
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

std::pair<size_t, Float> DiscreteDistribution1D::SampleWithPmf(Float xi) const {
  size_t index = Sample(xi);
  return std::make_pair(index, PmfNormalized(index));
}

std::pair<size_t, Float> DiscreteDistribution1D::SampleReuse(Float xi) const {
  size_t index = Sample(xi);
  Float pmf = PmfNormalized(index);
  Float cdf = index == 0 ? 0 : CdfNormalized(index - 1);
  return {index, (xi - cdf) / pmf};
}

ContinuousDistribution1D::ContinuousDistribution1D(const Float* pdf, size_t size) {
  _pdf = std::vector<Float>(pdf, pdf + size);
  _cdf.resize(size + 1);
  _cdf[0] = 0;
  size_t count = _pdf.size();
  for (int i = 1; i < count + 1; ++i) {  //求积分转化为求和
    _cdf[i] = _cdf[i - 1] + _pdf[i - 1] / count;
  }
  _sum = _cdf[count];
  for (int i = 1; i < count + 1; i++) {
    _cdf[i] /= _sum;
  }
  _normalization = Float(1.0 / _sum);
}

ContinuousDistribution1D::ContinuousDistribution1D(const std::vector<Float>& data)
    : ContinuousDistribution1D(data.data(), data.size()) {}

size_t ContinuousDistribution1D::Count() const {
  return _pdf.size();
}

Float ContinuousDistribution1D::PdfNormalized(size_t index) const {
  return _pdf[index] * _normalization;
}

Float ContinuousDistribution1D::Pdf(size_t index) const {
  return _pdf[index];
}

std::tuple<size_t, Float, Float> ContinuousDistribution1D::Sample(Float xi) const {
  auto iter = std::lower_bound(_cdf.begin(), _cdf.end(), xi);
  size_t index = std::distance(_cdf.begin(), iter);
  //获取小一些的cdf, 这样可以和大一些的cdf之间进行线性插值
  index = std::clamp(index - 1, size_t(0), _cdf.size() - 2);
  Float du = xi - _cdf[index];
  if ((_cdf[index + 1] - _cdf[index]) > 0) {
    du /= (_cdf[index + 1] - _cdf[index]);
  }
  Float pdf = PdfNormalized(index);
  Float cdf = (index + du) / Count();
  return std::make_tuple(index, cdf, pdf);
}

ContinuousDistribution2D::ContinuousDistribution2D(const std::vector<Float>& data, size_t nu, size_t nv)
    : ContinuousDistribution2D(data.data(), nu, nv) {}

ContinuousDistribution2D::ContinuousDistribution2D(const Float* data, size_t nu, size_t nv) {
  _conditional.resize(nv);
  for (size_t v = 0; v < nv; v++) {  //计算条件概率
    _conditional[v] = ContinuousDistribution1D(data + v * nu, nu);
  }
  std::vector<Float> marginal;
  marginal.resize(nv);
  for (int v = 0; v < nv; v++) {  //计算边缘分布
    marginal[v] = _conditional[v].Sum();
  }
  _marginal = ContinuousDistribution1D(marginal.data(), marginal.size());
}

std::tuple<Eigen::Vector2<size_t>, Vector2, Float> ContinuousDistribution2D::Sample(const Vector2& xi) const {
  auto [vj, d1, pdf1] = _marginal.Sample(xi.y());
  auto [vi, d0, pdf0] = _conditional[vj].Sample(xi.x());
  Float pdf = pdf0 * pdf1;
  Eigen::Vector2<size_t> index(vi, vj);
  Vector2 cdf(d0, d1);
  return std::make_tuple(index, cdf, pdf);
}

Float ContinuousDistribution2D::Pdf(const Vector2& uv) const {
  size_t iu = std::clamp(size_t(uv.x() * _conditional[0].Count()), size_t(0), _conditional[0].Count() - 1);
  size_t iv = std::clamp(size_t(uv.y() * _marginal.Count()), size_t(0), _marginal.Count() - 1);
  return _conditional[iv].Pdf(iu) / _marginal.Sum();
}

}  // namespace Rad
