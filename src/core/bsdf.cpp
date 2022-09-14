#include <radiantium/bsdf.h>

namespace rad {

bool BsdfContext::IsEnableType(UInt32 type) const {
  UInt32 t = (UInt32)type;
  return (Type & t) == t;
}

}  // namespace rad
