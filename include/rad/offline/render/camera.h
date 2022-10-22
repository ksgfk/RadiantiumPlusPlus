#pragma once

#include "../common.h"
#include "interaction.h"
#include "sample_result.h"

namespace Rad {

/**
 * @brief 相机里有frame buffer (胶卷), 采样器 (Sampler)
 */
class Camera {
 public:
  virtual ~Camera() noexcept = default;

  /**
   * @brief 分辨率
   */
  Eigen::Vector2i Resolution() const { return _resolution; }
  /**
   * @brief 世界空间的坐标
   */
  Vector3 WorldPosition() const { return _position; }
  MatrixX<Spectrum>& GetFrameBuffer() { return _fb; }
  const MatrixX<Spectrum>& GetFrameBuffer() const { return _fb; }
  /**
   * @brief 挂载一个采样器到相机上, 采样器的生命周期由相机管理
   */
  void AttachSampler(Unique<Sampler> sampler) { _sampler = std::move(sampler); }
  const Sampler& GetSampler() const { return *_sampler; }

  /**
   * @brief 根据屏幕空间坐标生成一条光线
   */
  virtual Ray SampleRay(const Vector2& screenPosition) const = 0;
  virtual RayDifferential SampleRayDifferential(const Vector2& screenPosition) const = 0;

  virtual std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const = 0;

  virtual std::tuple<DirectionSampleResult, Spectrum, Float, Float, Float> SampleDirectionWithPdf(
      const Interaction& ref,
      const Vector2& xi) const = 0;
  virtual std::pair<Float, Float> PdfDirection(const Ray& ray) const = 0;

 protected:
  /**
   * @brief 应该在相机构造完成后调用这个函数, 来生成frame buffer
   */
  inline void InitCamera() {
    _fb = MatrixX<Spectrum>();
    _fb.resize(_resolution.x(), _resolution.y());
  }

  Eigen::Vector2i _resolution;
  Vector3 _position;
  MatrixX<Spectrum> _fb;
  Unique<Sampler> _sampler;
};

}  // namespace Rad
