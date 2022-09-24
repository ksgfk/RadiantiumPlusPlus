#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>

#include <random>

namespace Rad {

SampleRenderer::SampleRenderer(BuildContext* ctx, const ConfigNode& cfg) {
  _logger = Logger::GetCategory("renderer");
  _scene = ctx->GetScene();
  _threadCount = cfg.ReadOrDefault("thread_count", -1);
}

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
          thread_local auto rng = std::mt19937(seed);
          std::uniform_real_distribution<Float> dist;
          Unique<Sampler> localSampler = sampler.Clone(sampler.GetSeed() + seed);
          for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
            for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
              for (UInt32 i = 0; i < sampler.SampleCount(); i++) {
                if (_isStop) {
                  break;
                }
                localSampler->Advance();
                Vector2 scrPos(x + dist(rng), y + dist(rng));
                Ray ray = camera.SampleRay(scrPos);
                Spectrum li = Li(ray, scene, localSampler.get());
                if (li.HasNaN() || li.HasInfinity() || li.HasNegative()) {
                  _logger->warn("invalid spectrum {}", li);
                } else {
                  frameBuffer(x, y) += li;
                }
              }
              _completeTask++;
            }
          }
          Float32 coeff = 1.0f / sampler.SampleCount();
          for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
            for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
              frameBuffer(x, y) *= coeff;
            }
          }
        },
        part);
    _sw.Stop();
  });
  _renderThread = std::make_unique<std::thread>(std::move(renderThread));
}

void SampleRenderer::Wait() {
  if (_renderThread->joinable()) {
    _renderThread->join();
  }
}

void SampleRenderer::Stop() {
  _isStop = true;
}

void SampleRenderer::SaveResult(const LocationResolver& resolver) const {
  resolver.SaveOpenExr(_scene->GetCamera().GetFrameBuffer());
}

}  // namespace Rad
