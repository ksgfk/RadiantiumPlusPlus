#pragma once

#include "radiantium.h"
#include "object.h"
#include "spectrum.h"
#include "interaction.h"

namespace rad {

enum class TransportMode {
  Radiance,
  Importance,
};

enum class BsdfType : UInt32 {
  Empty = 0b000000,
  Diffuse = 0b000001,
  Glossy = 0b000010,
  Delta = 0b000100,
  Reflection = 0b001000,
  Transmission = 0b010000,

  All = Diffuse | Glossy | Delta | Reflection | Transmission
};

struct BsdfContext {
  TransportMode Mode = TransportMode::Radiance;
  UInt32 Type = (UInt32)BsdfType::All;

  bool IsEnableType(BsdfType type) const;
};

struct BsdfSampleResult {
  Vec3 Wo;
  Float Pdf = 0;
  Float Eta = 1;
  UInt32 Type = (UInt32)BsdfType::Empty;
};

class IBsdf : public Object {
 public:
  virtual ~IBsdf() noexcept = default;

  virtual std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobe, const Vec2& dir) const = 0;

  virtual Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vec3& wo) const = 0;

  virtual Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vec3& wo) const = 0;

  virtual UInt32 Flags() const = 0;
};

}  // namespace rad
