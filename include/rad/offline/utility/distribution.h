#include "../common.h"

namespace Rad {

/**
 * @brief 一维离散随机变量的分布
 */
class DiscreteDistribution1D {
 public:
  DiscreteDistribution1D() = default;
  DiscreteDistribution1D(const std::vector<Float>& data);
  DiscreteDistribution1D(const Float* data, size_t size);

  /**
   * @brief 随机变量总和
   */
  Float Sum() const { return _sum; }
  /**
   * @brief 离散随机变量的归一化系数
   */
  Float Normalization() const { return _normalization; }

  /**
   * @brief 采样一个随机变量
   */
  size_t Sample(Float xi) const;
  /**
   * @brief 采样一个随机变量, 并返回样本的概率密度
   */
  std::pair<size_t, Float> SampleWithPdf(Float xi) const;
  /**
   * @brief 采样一个随机变量, 并且返回经过调整的原始样本, 让它可以重用
   */
  std::pair<size_t, Float> SampleReuse(Float xi) const;
  /**
   * @brief 返回样本的概率密度
   */
  Float Pdf(size_t index) const;

 private:
  void Update();

  /**
   * @brief 累计分布函数
   */
  std::vector<Float> _cdf;
  Float _sum = 0;
  Float _normalization = 0;
};

}  // namespace Rad
