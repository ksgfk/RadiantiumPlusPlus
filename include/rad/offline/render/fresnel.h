#include "../common.h"

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
class Fresnel {
 public:
  /**
   * @brief 反射, 假设传入方向是以(0, 0, 1)为法线的向量
   */
  static Vector3 Reflect(const Vector3& wi);
  /**
   * @brief 根据入射方向与给定法线计算反射方向
   */
  static Vector3 Reflect(const Vector3& wi, const Vector3& wh);

  /**
   * @brief 导体菲涅尔
   */
  static Vector3 Conductor(Float cosThetaI, const Vector3& eta, const Vector3& k);
};

}  // namespace Rad
