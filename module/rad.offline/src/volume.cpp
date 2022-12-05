#include <rad/offline/render/volume.h>

namespace Rad {

Volume::Volume(const Spectrum& spectrum)
    : _isConstVolume(true),
      _constValue(spectrum),
      _maxValue(spectrum.MaxComponent()) {}

Spectrum Volume::Eval(const Interaction& it) const {
  if (_isConstVolume) {
    return _constValue;
  }
  return EvalImpl(it);
}

Spectrum Volume::EvalImpl(const Interaction& it) const {
  throw RadInvalidOperationException("no volume eval impl");
}

}  // namespace Rad
