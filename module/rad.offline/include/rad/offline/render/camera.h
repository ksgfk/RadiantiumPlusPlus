#pragma once

#include "interaction.h"
#include "sample_result.h"
#include "sampler.h"

namespace Rad {

/**
 * @brief 相机里有frame buffer (胶卷), 采样器 (Sampler)
 */
class RAD_EXPORT_API Camera {
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
  /**
   * @brief 根据屏幕空间坐标生成一条带微分光线
   */
  virtual RayDifferential SampleRayDifferential(const Vector2& screenPosition) const = 0;

  /**
   * @brief 在摄像机成像面上采样一个方向
   *
   * @param ref 参考点
   * @param xi 随机数
   * @return std::pair<DirectionSampleResult, Spectrum> [采样结果, 这个方向上的相应比importance]
   */
  virtual std::pair<DirectionSampleResult, Spectrum> SampleDirection(
      const Interaction& ref,
      const Vector2& xi) const = 0;
  /**
   * @brief 评估射线方向上采样响应的pdf
   * @return std::pair<Float, Float> 面积上的pdf, 立体角方向上的pdf
   */
  virtual std::pair<Float, Float> PdfWe(const Ray& ray) const = 0;

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
