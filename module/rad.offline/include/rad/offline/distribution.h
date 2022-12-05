#pragma once

#include "types.h"

#include <vector>
#include <utility>

namespace Rad {

/**
 * @brief 一维离散随机变量的分布
 * Probability Mass Function 概率质量函数, 个人理解就是离散版本的pdf
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
   * @brief 归一化的概率质量函数
   */
  Float PmfNormalized(size_t index) const;
  /**
   * @brief 归一化的累积分布函数
   */
  Float CdfNormalized(size_t index) const;
  /**
   * @brief 样本数量
   */
  size_t Count() const;

  /**
   * @brief 采样一个随机变量
   */
  size_t Sample(Float xi) const;
  /**
   * @brief 采样一个随机变量, 并返回样本的概率密度
   */
  std::pair<size_t, Float> SampleWithPmf(Float xi) const;
  /**
   * @brief 采样一个随机变量, 并且返回经过调整的原始样本, 让它可以重用
   */
  std::pair<size_t, Float> SampleReuse(Float xi) const;

 private:
  /**
   * @brief 概率质量函数
   */
  std::vector<Float> _pmf;
  /**
   * @brief 累计分布函数
   */
  std::vector<Float> _cdf;
  Float _sum = 0;
  Float _normalization = 0;
};

/**
 * @brief 一维离散随机变量的分布, 采样时对变量线性插值
 */
class ContinuousDistribution1D {
 public:
  ContinuousDistribution1D() = default;
  ContinuousDistribution1D(const std::vector<Float>& data);
  ContinuousDistribution1D(const Float* data, size_t size);

  /**
   * @brief 随机变量总和
   */
  Float Sum() const { return _sum; }
  /**
   * @brief 离散随机变量的归一化系数
   */
  Float Normalization() const { return _normalization; }
  /**
   * @brief 样本数量
   */
  size_t Count() const;
  /**
   * @brief 归一化的概率密度累积分布函数
   */
  Float PdfNormalized(size_t index) const;
  /**
   * @brief 原始的概率密度分布
   */
  Float Pdf(size_t index) const;
  /**
   * @brief 采样
   *
   * @return std::tuple<size_t, Float, Float> 采样到的随机变量下标, cdf, pdf
   */
  std::tuple<size_t, Float, Float> Sample(Float xi) const;

 private:
  std::vector<Float> _pdf;
  std::vector<Float> _cdf;
  Float _sum = 0;
  Float _normalization = 0;
};

/**
 * @brief 二维离散随机变量的分布
 * https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#Piecewise-Constant2DDistributions
 * 二维需要使用条件概率密度
 */
class ContinuousDistribution2D {
 public:
  ContinuousDistribution2D() = default;
  ContinuousDistribution2D(const std::vector<Float>& data, size_t nu, size_t nv);
  ContinuousDistribution2D(const Float* data, size_t nu, size_t nv);

  std::tuple<Eigen::Vector2<size_t>, Vector2, Float> Sample(const Vector2& xi) const;
  Float Pdf(const Vector2& uv) const;

 private:
  std::vector<ContinuousDistribution1D> _conditional;
  ContinuousDistribution1D _marginal;
};

}  // namespace Rad
