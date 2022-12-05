#pragma once

#include <rad/core/config_node.h>
#include <rad/core/location_resolver.h>
#include <rad/core/stop_watch.h>
#include <rad/offline/fwd.h>
#include <rad/offline/types.h>
#include <rad/offline/ray.h>
#include <rad/offline/render/scene.h>

#include <thread>
#include <atomic>

namespace Rad {

/**
 * @brief 渲染器抽象
 */
class RAD_EXPORT_API Renderer {
 public:
  Renderer(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg);
  virtual ~Renderer() noexcept = default;

  bool IsComplete() const { return _isComplete; }
  UInt64 AllTaskCount() const { return _allTask; }
  UInt64 CompletedTaskCount() const { return _completeTask; }
  /**
   * @brief 从开始渲染到现在已经经过的时间
   */
  Int64 ElapsedTime() const { return _sw.ElapsedMilliseconds(); }

  virtual void Start() = 0;
  virtual void Wait();
  virtual void Stop();
  virtual void SaveResult(const LocationResolver& resolver) const;

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

/**
 * @brief 假设光线都是从相机出发的渲染器
 */
class RAD_EXPORT_API SampleRenderer : public Renderer {
 public:
  SampleRenderer(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg);
  ~SampleRenderer() noexcept override;

  void Start() override;

 protected:
  virtual Spectrum Li(const RayDifferential& ray, const Scene& scene, Sampler* sampler) const = 0;
};

}  // namespace Rad
