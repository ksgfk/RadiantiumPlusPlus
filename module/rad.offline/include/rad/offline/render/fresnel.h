#pragma once

#include <rad/offline/types.h>

#include <optional>

namespace Rad {

/**
 * @brief https://pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission#FresnelReflectance
 * 菲涅尔描述了不同入射角度上, 物体表面反射光线能量的大小
 * 给定折射率和入射光线与表面法线的夹角, 菲涅耳方程指定了材料对入射照明的两种不同偏振态的相应反射率
 * 但在这里我们假设光是非偏振的, 因此菲涅尔反射率是平行与垂直偏振项的平方的平均值
 * 可以将大多数物体分为两种:
 * 1. 电介质, 折射率是实数, 例如玻璃, 水. 这类材质通常具有一定的光线透射能力
 * 2. 导体, 折射率是复数, 虚数部分被称为吸收系数, 例如铜, 金子,
 *    这类材质会反射绝大部分光线, 也有一小部分光线会被表面吸收, 这里也忽略这种现象
 * 事实上它们都可以由菲涅尔方程导出, 但由于电介质只有实数部分, 因此分离出来可以简化计算
 */
class RAD_EXPORT_API Fresnel {
 public:
  /**
   * @brief 反射, 假设法线是(0, 0, 1)
   */
  static Vector3 Reflect(const Vector3& wi);
  /**
   * @brief 根据入射方向与给定法线计算反射方向
   */
  static Vector3 Reflect(const Vector3& wi, const Vector3& wh);
  /**
   * @brief 折射, 假设法线是(0, 0, 1)
   */
  static Vector3 Refract(const Vector3& wi, Float cosThetaT, Float etaTI);
  /**
   * @brief 折射
   *
   * @param wi 折射方向
   * @param wh 表面法线
   * @param cosThetaT 透射方向与法线之间夹角的余弦值
   * @param etaTI 折射率的倒数
   */
  static Vector3 Refract(const Vector3& wi, const Vector3& wh, Float cosThetaT, Float etaTI);
  /**
   * @brief 折射
   *
   * @param wi 入射方向
   * @param wh 法线
   * @param eta 折射率
   */
  static std::optional<Vector3> Refract(const Vector3& wi, const Vector3& wh, Float eta);

  /**
   * @brief 电介质菲涅尔

   * @return std::tuple<Float, Float, Float, Float>
   * F 菲涅尔系数
   * cosThetaT 表面法线与透射光线之间夹角的余弦
   * etaIT 出射方向相对折射率
   * etaTI 出射方向相对折射率的倒数
   */
  static std::tuple<Float, Float, Float, Float> Dielectric(Float cosThetaI, Float eta);
  /**
   * @brief 导体菲涅尔
   */
  static Spectrum Conductor(Float cosThetaI, const Spectrum& eta, const Spectrum& k);
  /**
   * @brief 计算电介质在表面的漫反射比例
   */
  static Float DiffuseReflectance(Float eta);
};

}  // namespace Rad
