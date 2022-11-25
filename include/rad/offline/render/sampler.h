#pragma once

#include "../common.h"
#include "../build/config_node.h"

namespace Rad {

/**
 * @brief 采样器提供了生成随机样本的能力, 这些样本的范围在[0,1)内
 * 首先需要调用SetSeed来初始化采样器
 * 然后就可以获取样本了, 当每一轮采样完成后需要调用 Advance() 函数
 */
class RAD_EXPORT_API Sampler {
 public:
  inline Sampler(BuildContext* ctx, const ConfigNode& cfg) {
    _sampleCount = cfg.ReadOrDefault("sample_count", 16);
  }
  virtual ~Sampler() noexcept = default;

  UInt32 SampleCount() const { return _sampleCount; }

  virtual Unique<Sampler> Clone(UInt32 seed) const = 0;

  UInt32 GetSeed() const { return _seed; }
  virtual void SetSeed(UInt32 seed) = 0;

  virtual Float Next1D() = 0;
  virtual Vector2 Next2D() = 0;
  virtual Vector3 Next3D() = 0;
  inline virtual void Advance() {}

 protected:
  UInt32 _sampleCount;
  UInt32 _seed;
};

}  // namespace Rad
