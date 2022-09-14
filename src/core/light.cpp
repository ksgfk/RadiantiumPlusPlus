#include <radiantium/light.h>

namespace rad {

bool ILight::HasFlag(LightFlag flag) const {
  return (Flags() & (UInt32)flag) != 0;
}

bool ILight::IsEnv() const {
  return HasFlag(LightFlag::Infinite) && !HasFlag(LightFlag::Delta);
}

}  // namespace rad
