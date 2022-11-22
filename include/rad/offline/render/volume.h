#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

class Volume {
 public:
  Volume() : _isConstVolume(false) {}
  Volume(const Spectrum& spectrum) : _isConstVolume(true), _constValue(spectrum) {}
  virtual ~Volume() noexcept = default;

  Spectrum Eval(const Interaction& it) const;
  const BoundingBox3& GetBoundingBox() const { return _box; }

 protected:
  virtual Spectrum EvalImpl(const Interaction& it) const = 0;

 private:
  bool _isConstVolume;
  Spectrum _constValue;
  Transform _toWorld;
  BoundingBox3 _box;
};

}  // namespace Rad