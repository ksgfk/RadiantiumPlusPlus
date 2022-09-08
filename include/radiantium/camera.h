#pragma once

#include "radiantium.h"
#include "ray.h"
#include "object.h"
#include "fwd.h"

#include <memory>

namespace rad {
/**
 * @brief 摄像机抽象
 */
class ICamera : public Object {
 public:
  virtual ~ICamera() noexcept = default;

  /*属性*/
  virtual Eigen::Vector2i Resolution() const = 0;
  virtual Vec3 WorldPosition() const = 0;

  /*方法*/
  virtual Ray SampleRay(Vec2 screenPosition) const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreatePerspectiveFactory(); //透视相机
}

}  // namespace rad
