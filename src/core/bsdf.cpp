#include <radiantium/bsdf.h>

namespace rad {

bool BsdfContext::IsEnableType(BsdfType type) const {
  UInt32 t = (UInt32)type;
  return (Type & t) == t;
}

}  // namespace rad
