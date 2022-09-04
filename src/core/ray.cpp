#include <radiantium/ray.h>

namespace rad {

Vec3 Ray::operator()(Float t) const {
  return O + D * t;
}

}  // namespace rad
