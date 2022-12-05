#pragma once

#include "interaction.h"

namespace Rad {

/**
 * @brief 体积，是一种三维数组，记录了三维空间中的点包含的数据
 * 采样时参数范围[0,1]^3
 */
class RAD_EXPORT_API Volume {
 public:
  Volume() : _isConstVolume(false) {}
  Volume(const Spectrum& spectrum);
  virtual ~Volume() noexcept = default;

/**
 * @brief 评估体积使用it.P作为采样参数
 */
  Spectrum Eval(const Interaction& it) const;
  const Float GetMaxValue() const { return _maxValue; }

 protected:
  virtual Spectrum EvalImpl(const Interaction& it) const;

  bool _isConstVolume;
  Spectrum _constValue;
  Float _maxValue;
};

}  // namespace Rad
