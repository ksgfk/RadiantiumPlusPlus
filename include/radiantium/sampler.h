#pragma once

#include <memory>

#include "object.h"
#include "radiantium.h"

namespace rad {

class ISampler : public Object {
 public:
  virtual ~ISampler() noexcept = default;

  virtual UInt32 Seed() const = 0;

  virtual std::unique_ptr<ISampler> Clone() const = 0;

  virtual Float Next1D() = 0;
  virtual Vec2 Next2D() = 0;
  virtual void SetSeed(UInt32 seed) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateIndependentSamplerFactory();
}

}  // namespace rad
