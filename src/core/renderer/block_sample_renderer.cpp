#include <radiantium/renderer.h>

#include <random>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>

namespace rad {

BlockSampleRenderer::BlockSampleRenderer(
    std::unique_ptr<World> world,
    Int32 spp,
    Int32 taskCount)
    : _world(std::move(world)),
      _samplePerPixel(spp),
      _taskCount(taskCount) {
  Eigen::Vector2i res = _world->GetCamera()->Resolution();
  _frameBuffer = std::make_unique<StaticBuffer2D<Spectrum>>(res.x(), res.y());
  _allTask = res.x() * res.y();
  _isComplete = false;
}

bool BlockSampleRenderer::IsComplete() const {
  return _isComplete;
}

UInt32 BlockSampleRenderer::AllTaskCount() const {
  return _allTask;
}

UInt32 BlockSampleRenderer::CompletedTaskCount() const {
  return _complete;
}

Int64 BlockSampleRenderer::ElapsedTime() const {
  return _sw.ElapsedMilliseconds();
}

const std::thread& BlockSampleRenderer::Start() {
  if (_renderThread != nullptr) {
    return *_renderThread;
  }
  _sw.Start();
  std::thread renderThread([&]() {
    Eigen::Vector2i res = _world->GetCamera()->Resolution();
    tbb::affinity_partitioner part;
    std::unique_ptr<tbb::global_control> ctrl;
    if (_taskCount > 0) {
      ctrl = std::make_unique<tbb::global_control>(tbb::global_control::max_allowed_parallelism, _taskCount);
    }
    tbb::parallel_for(
        tbb::blocked_range2d<UInt32>(0, res.y(), 0, res.x()),
        [&](const tbb::blocked_range2d<UInt32>& r) {
          World* world = _world.get();
          const ICamera* camera = world->GetCamera();
          thread_local auto sampler = world->GetSampler()->Clone();
          thread_local auto engine = std::mt19937(r.cols().begin() * r.rows().begin());
          std::uniform_real_distribution<Float> dist;
          sampler->SetSeed(sampler->Seed() + r.cols().begin() * r.rows().begin());
          for (UInt32 y = r.rows().begin(); y != r.rows().end(); y++) {
            auto [start, _] = _frameBuffer->GetRowSpan(y);
            for (UInt32 x = r.cols().begin(); x != r.cols().end(); x++) {
              for (Int32 i = 0; i < _samplePerPixel; i++) {
                Vec2 scrPos(x + dist(engine), y + dist(engine));
                Ray ray = camera->SampleRay(scrPos);
                Spectrum li = Li(ray, world, sampler.get());
                if (li.HasNaN() || li.HasInfinity() || li.HasNegative()) {
                  logger::GetLogger()->warn("invalid spectrum {}", li);
                } else {
                  start[x] += li;
                }
              }
              _complete++;
            }
          }
          for (UInt32 y = r.rows().begin(); y != r.rows().end(); y++) {
            auto [start, _] = _frameBuffer->GetRowSpan(y);
            for (UInt32 x = r.cols().begin(); x != r.cols().end(); x++) {
              start[x] /= (Float)_samplePerPixel;
            }
          }
        },
        part);
    _isComplete = true;
    _sw.Stop();
  });
  _renderThread = std::make_unique<std::thread>(std::move(renderThread));
  return *_renderThread;
}

void BlockSampleRenderer::Wait() {
  _renderThread->join();
}

void BlockSampleRenderer::Stop() {
  // TODO:
}

void BlockSampleRenderer::SaveResult(const DataWriter& resolver) const {
  resolver.SaveExr(*_frameBuffer);
}

}  // namespace rad
