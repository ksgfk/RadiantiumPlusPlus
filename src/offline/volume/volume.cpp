#include <rad/offline/render/volume.h>

namespace Rad {

Spectrum Volume::Eval(const Interaction& it) const {
  if (_isConstVolume) {
    return _constValue;
  }
  return EvalImpl(it);
}

void Volume::UpdateBoundingBox() {
  BoundingBox3 box(Vector3::Zero(), Vector3::Constant(1));
  _box = _toWorld.ApplyBoxToWorld(box);
}

}  // namespace Rad
