#include <rad/offline/render/renderer.h>

#include <rad/core/logger.h>
#include <rad/core/image_reader.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/render/scene.h>
#include <rad/offline/render/camera.h>
#include <rad/offline/render/sampler.h>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>

#include <random>

namespace Rad {

Renderer::Renderer(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg) {
  _logger = Logger::GetCategory("renderer");
  _scene = std::move(scene);
  _threadCount = cfg.ReadOrDefault("thread_count", -1);
}

void Renderer::Wait() {
  if (_renderThread->joinable()) {
    _renderThread->join();
  }
}

void Renderer::Stop() {
  _isStop = true;
}

void Renderer::SaveResult(const LocationResolver& resolver) const {
  auto&& fb = _scene->GetCamera().GetFrameBuffer();
  MatrixX<Color24f> tmp(fb.rows(), fb.cols());
  for (UInt32 y = 0; y < tmp.cols(); y++) {
    for (UInt32 x = 0; x < tmp.rows(); x++) {
      tmp.coeffRef(x, y) = fb.coeff(x, y).cast<Float32>();
    }
  }
  auto stream = resolver.WriteStream(resolver.GetSaveName("exr"), std::ios::binary | std::ios::out);
  ImageReader::WriteExr(*stream, tmp);
}

SampleRenderer::SampleRenderer(
    BuildContext* ctx,
    Unique<Scene> scene,
    const ConfigNode& cfg)
    : Renderer(ctx, std::move(scene), cfg) {}

SampleRenderer::~SampleRenderer() noexcept {
  if (_renderThread->joinable()) {
    _renderThread->join();
  }
}

void SampleRenderer::Start() {
  if (_renderThread != nullptr) {
    return;
  }
  std::thread renderThread([&]() {
    Scene& scene = *_scene;
    Camera& camera = scene.GetCamera();
    const Sampler& sampler = camera.GetSampler();
    MatrixX<Spectrum>& frameBuffer = camera.GetFrameBuffer();
    _allTask = frameBuffer.cols() * frameBuffer.rows();
    tbb::affinity_partitioner part;
    std::unique_ptr<tbb::global_control> ctrl;
    if (_threadCount > 0) {
      ctrl = std::make_unique<tbb::global_control>(tbb::global_control::max_allowed_parallelism, _threadCount);
    }
    tbb::blocked_range2d<UInt32> block(
        0, camera.Resolution().x(),
        0, camera.Resolution().y());
    _sw.Start();
    tbb::parallel_for(
        block, [&](const tbb::blocked_range2d<UInt32>& r) {
          UInt32 seed = r.rows().begin() * camera.Resolution().x() + r.cols().begin();
          std::mt19937 rng(seed);
          std::uniform_real_distribution<Float> dist;
          Unique<Sampler> localSampler = sampler.Clone(sampler.GetSeed() + seed);
          for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
            if (_isStop) {
              break;
            }
            for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
              if (_isStop) {
                break;
              }
              for (UInt32 i = 0; i < sampler.SampleCount(); i++) {
                if (_isStop) {
                  break;
                }
                localSampler->Advance();
                Vector2 scrPos(x + dist(rng), y + dist(rng));
                RayDifferential ray = camera.SampleRayDifferential(scrPos);
                Spectrum li = Li(ray, scene, localSampler.get());
                if (li.HasNaN() || li.HasInfinity() || li.HasNegative()) {
                  _logger->warn("invalid spectrum {}", li);
                } else {
                  frameBuffer(x, y) += li;
                }
              }
            }
          }
          Float32 coeff = 1.0f / sampler.SampleCount();
          for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
            for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
              frameBuffer(x, y) *= coeff;
            }
          }
          _completeTask += r.cols().size() * r.rows().size();
        },
        part);
    _sw.Stop();
    _isComplete = true;
  });
  _renderThread = std::make_unique<std::thread>(std::move(renderThread));
}

}  // namespace Rad
