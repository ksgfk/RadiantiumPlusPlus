#include "../common.h"

namespace Rad {

enum class MicrofacetType {
  Beckmann,
  GGX
};

/**
 * @brief
 * https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models
 * 粗糙表面可以被看作是很多微小镜面的集合, 使用统计方法对这些镜面建模的结果就是微表面模型
 * 微表面模型由两个部分组成: 微小镜面的分布情况, 光如何在镜面上散射
 *
 * 分布情况由分布函数 D(wh) 描述, 为了保证物理合理性, 这个函数需要归一化: ∫ D(wh) * cosThetaH dwh = 1, 积分上下限是半球
 * 说人话就是, 给定微表面的微分面积 dA, 则该区域上方的微平面面的投影面积必须等于 dA
 * 再说得像人话一些, 微表面法线分布在表面的上半球上, 把这个半球拍扁是个圆, 这个圆的面积必须等于单位面积
 *
 * 由于表面的粗糙, 入射光可能被掩蔽(Masking), 出射方向可能被隐藏(Shadowing), 光可能在微面间弹射(Interreflection)
 * 一个常用的描述掩蔽和隐藏的函数是 Smith’s masking-shadowing function G1(w, wh), 它给出了从方向 w 射入微表面, 出射部分可见的比例
 * G1需要满足: ∫ G1(w, wh)max(0, w · wh)D(wh) dwh = cosTheta, 积分上下限是半球
 * 说人话就是, 给定方向w, 可见的微表面的投影面积必须等于 dA
 * G1函数传统上使用辅助函数 Lambda(w) 表示: G1(w) = 1 / (1 + Lambda(w))
 *
 * 除了G1之外，Eric Heitz在[Heitz 2014]中提出的Smith联合遮蔽阴影函数G2(wi, wo, wh)来代替遮蔽函数G1(w, wh)
 * 最广泛使用的是Separable Masking and Shadowing Function, 不模拟遮蔽和阴影之间的相关性
 *
 * 上面都在讨论Masking和Shadowing, 那Interreflection呢? 可以直接忽略掉, 只是能量不守恒了, 出射能量会变少
 * 但是大佬做了详细的研究: https://eheitzresearch.wordpress.com/240-2/
 *
 * 微表面也可以重要性采样, 无论是Beckmann还是GGX的D(wh)都可以使用inverse method直接推出样本变换的公式
 * 但是shadowing-masking就不太行了, 想推导inverse method太难了
 * http://jcgt.org/published/0007/04/01/paper.pdf
 */
template <typename T>
class MicrofacetDistribution {  // Curiously Recurring Template Pattern (CRTP)
 public:
  Float D(const Vector3& wh) const {
    return static_cast<const T*>(this)->DImpl(wh);
  }
  Float G(const Vector3& wi, const Vector3& wo, const Vector3& wh) const {
    return static_cast<const T*>(this)->GImpl(wi, wo, wh);
  }
  Float SmithG1(const Vector3& v, const Vector3& wh) const {
    return static_cast<const T*>(this)->SmithG1Impl(v, wh);
  }
  Float Pdf(const Vector3& wi, const Vector3& wh) const {
    return static_cast<const T*>(this)->PdfImpl(wi, wh);
  }
  std::pair<Vector3, Float> Sample(const Vector3& wi, const Vector2& xi) const {
    return static_cast<const T*>(this)->SampleImpl(wi, xi);
  }

  Float alphaX, alphaY;
};

class Beckmann final : public MicrofacetDistribution<Beckmann> {
 public:
  Float DImpl(const Vector3& wh) const;
  Float SmithG1Impl(const Vector3& v, const Vector3& wh) const;
  Float GImpl(const Vector3& wi, const Vector3& wo, const Vector3& wh) const;
  Float PdfImpl(const Vector3& wi, const Vector3& wh) const;
  std::pair<Vector3, Float> SampleImpl(const Vector3& wi, const Vector2& xi) const;

  bool sampleVisible;
};

class GGX final : public MicrofacetDistribution<GGX> {
 public:
  Float DImpl(const Vector3& wh) const;
  Float SmithG1Impl(const Vector3& v, const Vector3& wh) const;
  Float GImpl(const Vector3& wi, const Vector3& wo, const Vector3& wh) const;
  Float PdfImpl(const Vector3& wi, const Vector3& wh) const;
  std::pair<Vector3, Float> SampleImpl(const Vector3& wi, const Vector2& xi) const;

  bool sampleVisible;
};

}  // namespace Rad
