#pragma once

#include "radiantium.h"
#include "object.h"
#include "spectrum.h"
#include "interaction.h"

#include <memory>

namespace rad {

enum class LightFlag : UInt32 {
  Empty = 0b000000,
  DeltaPosition = 0x000001,
  DeltaDirection = 0x000010,
  Infinite = 0x000100,
  Surface = 0x001000,

  Delta = DeltaPosition | DeltaDirection,
};

class ILight : public Object {
 public:
  virtual ~ILight() noexcept = default;

  virtual UInt32 Flags() const = 0;
  bool HasFlag(LightFlag flag) const;
  bool IsEnv() const;

  virtual Spectrum Eval(const SurfaceInteraction& si) const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateAreaLightFactory();
}  // namespace factory

}  // namespace rad
