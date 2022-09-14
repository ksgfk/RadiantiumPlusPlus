#pragma once

#include "radiantium.h"
#include "spectrum.h"

namespace rad {

class ITexture {
 public:
  virtual ~ITexture() noexcept = default;

  virtual RgbSpectrum Sample(const Vec2& uv) const = 0;
  virtual RgbSpectrum Read(UInt32 x, UInt32 y) const = 0;

  virtual UInt32 Width() const = 0;
  virtual UInt32 Height() const = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateBitmapFactory();
std::unique_ptr<IFactory> CreateConstTextureFactory();
}  // namespace factory

}  // namespace rad
