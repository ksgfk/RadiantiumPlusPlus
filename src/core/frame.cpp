#include <radiantium/frame.h>

#include <radiantium/math_ext.h>

namespace rad {

Frame::Frame(const Vec3& n) noexcept {
  std::tie(S, T) = math::CoordinateSystem(n);
}

}  // namespace rad
