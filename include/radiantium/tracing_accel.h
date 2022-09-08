#pragma once

#include "ray.h"
#include "interaction.h"
#include "shape.h"
#include "object.h"
#include "fwd.h"

namespace rad {
/**
 * @brief 光线追踪加速结构抽象
 *
 */
class ITracingAccel : public Object {
 public:
  virtual ~ITracingAccel() noexcept {}
  /*属性*/
  virtual bool IsValid() = 0;
  virtual const std::vector<IShape*> Shapes() noexcept = 0;

  /**
   * @brief 构建加速结构
   */
  virtual void Build(std::vector<IShape*>&& shapes) = 0;
  /**
   * @brief shadow ray
   */
  virtual bool RayIntersect(const Ray& ray) = 0;
  /**
   * @brief 初步求交
   *
   * @param hsr out 返回求交结果
   */
  virtual bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateEmbreeFactory();  // embrees
}

}  // namespace rad
