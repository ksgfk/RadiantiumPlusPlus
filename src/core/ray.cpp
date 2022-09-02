#include <radiantium/ray.h>

namespace rad {

Vec3 Ray::operator()(Float t) const {
  return o + d * t;
}

}  // namespace rad
