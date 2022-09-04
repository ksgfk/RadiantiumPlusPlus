#pragma once

#include "radiantium.h"
#include "ray.h"

#include <memory>

namespace rad {

class ICamera {
 public:
  virtual ~ICamera() noexcept = default;

  /* Properties */
  virtual Eigen::Vector2i Resolution() = 0;
  virtual Vec3 WorldPosition() = 0;

  /* Methods */
  virtual Ray SampleRay(Vec2 screenPosition) = 0;
};

std::unique_ptr<ICamera> CreatePerspective(
    Float fov, Float near, Float far,
    const Vec3& origin, const Vec3& target, const Vec3& up,
    const Eigen::Vector2i& resolution);

}  // namespace rad
