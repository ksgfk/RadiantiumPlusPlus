#include "../common.h"

namespace Rad {

class Fresnel {
 public:
  static Vector3 Reflect(const Vector3& wi);
  static Vector3 Reflect(const Vector3& wi, const Vector3& wh);

  static Vector3 Conductor(Float cosThetaI, const Vector3& eta, const Vector3& k);
};

}  // namespace Rad
