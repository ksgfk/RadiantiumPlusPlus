#pragma once

#include "radiantium.h"
#include "ray.h"
#include "object.h"
#include "fwd.h"

#include <memory>

namespace rad {

class ICamera : public Object {
 public:
  virtual ~ICamera() noexcept = default;

  /* Properties */
  virtual Eigen::Vector2i Resolution() const = 0;
  virtual Vec3 WorldPosition() const = 0;

  /* Methods */
  virtual Ray SampleRay(Vec2 screenPosition) const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreatePerspectiveFactory();
}

}  // namespace rad
