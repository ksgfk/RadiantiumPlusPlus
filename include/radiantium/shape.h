#pragma once

#include <cstdint>
#include <stdint.h>
#include <memory>

#include "radiantium.h"
#include "model.h"
#include "transform.h"
#include "interaction.h"
#include "object.h"
#include "fwd.h"

namespace rad {
/**
 * @brief 形状的抽象
 *
 */
class IShape : public Object {
 public:
  virtual ~IShape() noexcept = default;
  /*属性*/
  /**
   * @brief 形状包含的图元数量
   */
  virtual size_t PrimitiveCount() = 0;
  /*方法*/
  /**
   * @brief 将形状提交到 embree 设备
   *
   * @param device embree设备指针
   * @param scene embree场景
   * @param id 图元的唯一id
   */
  virtual void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) = 0;
  /**
   * @brief 从求交结果计算表面交点数据
   *
   * @param ray 求交射线
   * @param rec 求交结果
   * @return SurfaceInteraction
   */
  virtual SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateMeshFactory();  //三角形网格
std::unique_ptr<IFactory> CreateSphereFactory();
}

}  // namespace rad
