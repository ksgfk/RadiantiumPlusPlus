#pragma once

#include "interaction.h"

namespace Rad {

/**
 * @brief 求交加速结构的抽象
 */
class RAD_EXPORT_API Accel {
 public:
  virtual ~Accel() noexcept = default;

  /**
   * @brief 获取整个世界的包围盒
   */
  virtual BoundingBox3 GetWorldBound() const = 0;

  /**
   * @brief shadow ray, 检查射线是否与场景中任何一个 shape 有交点
   */
  virtual bool RayIntersect(const Ray& ray) const = 0;
  /**
   * @brief 光线求交, 如果射线与场景中任何一个 shape 有交点, 就会返回true, 并且hsr会返回
   * 距离光线起点最近的物体的求交数据
   *
   * @param hsr [out] 返回最近的物体的求交数据
   */
  virtual bool RayIntersectPreliminary(const Ray& ray, HitShapeRecord& hsr) const = 0;
};

}  // namespace Rad
