#pragma once

#include "radiantium.h"
#include "location_resolver.h"
#include "spectrum.h"
#include "ray.h"
#include "stop_watch.h"
#include "world.h"
#include "static_buffer.h"
#include "factory.h"

#include <thread>

namespace rad {

class IRenderer {
 public:
  virtual ~IRenderer() noexcept = default;

  virtual bool IsComplete() const = 0;
  virtual UInt32 AllTaskCount() const = 0;
  virtual UInt32 CompletedTaskCount() const = 0;
  virtual Int64 ElapsedTime() const = 0;

  virtual const std::thread& Start() = 0;
  virtual void Wait() = 0;
  virtual void Stop() = 0;
  virtual void SaveResult(const DataWriter& resolver) const = 0;
};

class BlockSampleRenderer : public IRenderer {
 public:
  BlockSampleRenderer(std::unique_ptr<World> world, Int32 spp, Int32 taskCount);
  virtual ~BlockSampleRenderer() noexcept = default;

  bool IsComplete() const override;
  UInt32 AllTaskCount() const override;
  UInt32 CompletedTaskCount() const override;
  Int64 ElapsedTime() const override;

  const std::thread& Start() override;
  void Wait() override;
  void Stop() override;
  void SaveResult(const DataWriter& resolver) const override;

 protected:
  virtual Spectrum Li(const Ray& ray, const World* world, ISampler* sampler) = 0;

  std::unique_ptr<World> _world;
  std::unique_ptr<std::thread> _renderThread;
  Int32 _samplePerPixel;
  Int32 _taskCount;
  UInt32 _allTask;
  std::atomic_uint32_t _complete;
  Stopwatch _sw;
  bool _isComplete;
  std::unique_ptr<StaticBuffer2D<Spectrum>> _frameBuffer;
};

namespace factory {
std::unique_ptr<IFactory> CreateAORenderer();
}
}  // namespace rad
