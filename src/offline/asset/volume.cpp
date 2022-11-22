#include <rad/offline/render/volume.h>

namespace Rad {

Spectrum Volume::Eval(const Interaction& it) const {
  if (_isConstVolume) {
    return _constValue;
  }
  return EvalImpl(it);
}

}  // namespace Rad
