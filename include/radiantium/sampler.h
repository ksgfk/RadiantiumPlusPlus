#pragma once

#include <memory>

#include "object.h"
#include "radiantium.h"

namespace rad {
/**
 * @brief 采样器抽象
 *
 */
class ISampler : public Object {
 public:
  virtual ~ISampler() noexcept = default;

  virtual UInt32 Seed() const = 0;
  /**
   * @brief 复制一个采样器
   */
  virtual std::unique_ptr<ISampler> Clone() const = 0;

  virtual Float Next1D() = 0;
  virtual Vec2 Next2D() = 0;
  /**
   * @brief 设置采样器的随机数种子, 如果可用的话
   */
  virtual void SetSeed(UInt32 seed) = 0;
};

namespace factory {
std::unique_ptr<IFactory> CreateIndependentSamplerFactory();  //实际就是最简单的随机数发生器
}

}  // namespace rad
