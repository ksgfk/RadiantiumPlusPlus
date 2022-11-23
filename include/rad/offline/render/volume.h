#pragma once

#include "../common.h"
#include "interaction.h"

namespace Rad {

class Volume {
 public:
  Volume() : _isConstVolume(false) {}
  inline Volume(const Spectrum& spectrum)
      : _isConstVolume(true), _constValue(spectrum), _maxValue(spectrum.MaxComponent()) {}
  virtual ~Volume() noexcept = default;

  Spectrum Eval(const Interaction& it) const;
  const Float GetMaxValue() const { return _maxValue; }

 protected:
  virtual Spectrum EvalImpl(const Interaction& it) const;

  bool _isConstVolume;
  Spectrum _constValue;
  Float _maxValue;
};

}  // namespace Rad
