#pragma once

#include "../common.h"

namespace Rad {

class Camera {
 public:
  virtual ~Camera() noexcept = default;

  Eigen::Vector2i Resolution() const { return _resolution; }
  Vector3 WorldPosition() const { return _position; }
  MatrixX<Spectrum>& GetFrameBuffer() { return _fb; }
  const MatrixX<Spectrum>& GetFrameBuffer() const { return _fb; }
  void AttachSampler(Unique<Sampler> sampler) { _sampler = std::move(sampler); }
  const Sampler& GetSampler() const { return *_sampler; }

  virtual Ray SampleRay(Vector2 screenPosition) const = 0;

 protected:
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
