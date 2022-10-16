#pragma once

#include "../common.h"
#include "../utility/location_resolver.h"
#include "../utility/stop_watch.h"
#include "../build/config_node.h"

#include <thread>
#include <atomic>

namespace Rad {

/**
 * @brief 渲染器抽象
 *
 */
class Renderer {
 public:
  Renderer(BuildContext* ctx, const ConfigNode& cfg);
  virtual ~Renderer() noexcept = default;

  bool IsComplete() const { return _isComplete; }
  UInt64 AllTaskCount() const { return _allTask; }
  UInt64 CompletedTaskCount() const { return _completeTask; }
  Int64 ElapsedTime() const { return _sw.ElapsedMilliseconds(); }

  virtual void Start() = 0;
  virtual void Wait();
  virtual void Stop();
  virtual void SaveResult(const LocationResolver& resolver) const = 0;

 protected:
  Share<spdlog::logger> _logger;
  Unique<Scene> _scene;
  Unique<std::thread> _renderThread;
  Int32 _threadCount;
  UInt64 _allTask = 0;
  std::atomic_uint64_t _completeTask = 0;  //已完成任务数量
  Stopwatch _sw{};
  bool _isComplete = false;
  bool _isStop = false;
};

class SampleRenderer : public Renderer {
 public:
  SampleRenderer(BuildContext* ctx, const ConfigNode& cfg);
  ~SampleRenderer() noexcept override;

  void Start() override;
  virtual void SaveResult(const LocationResolver& resolver) const override;

 protected:
  virtual Spectrum Li(const RayDifferential& ray, const Scene& scene, Sampler* sampler) const = 0;
};

}  // namespace Rad
