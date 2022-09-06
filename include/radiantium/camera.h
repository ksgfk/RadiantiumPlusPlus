#pragma once

#include "radiantium.h"
#include "ray.h"
#include "object.h"

#include <memory>

namespace rad {

class IFactory;
class Object;

class ICamera : public Object {
 public:
  virtual ~ICamera() noexcept = default;

  /* Properties */
  virtual Eigen::Vector2i Resolution() = 0;
  virtual Vec3 WorldPosition() = 0;

  /* Methods */
  virtual Ray SampleRay(Vec2 screenPosition) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreatePerspectiveFactory();
}

}  // namespace rad
