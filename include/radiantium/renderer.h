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
/**
 * @brief 渲染器抽象
 *
 */
class IRenderer {
 public:
  virtual ~IRenderer() noexcept = default;
  /*属性*/
  virtual bool IsComplete() const = 0;
  virtual UInt32 AllTaskCount() const = 0;
  virtual UInt32 CompletedTaskCount() const = 0;
  /**
   * @brief 从开始经过的时间, 单位毫秒
   */
  virtual Int64 ElapsedTime() const = 0;
  /*方法*/
  virtual const std::thread& Start() = 0;
  virtual void Wait() = 0;
  virtual void Stop() = 0;
  virtual void SaveResult(const DataWriter& resolver) const = 0;
};

/**
 * @brief 分块采样渲染器
 */
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
  Int32 _samplePerPixel;  //每像素采样数量
  Int32 _taskCount;       //线程数量, -1表示使用推荐数量
  UInt32 _allTask;
  std::atomic_uint32_t _complete;  //已完成任务数量
  Stopwatch _sw;
  bool _isComplete;
  std::unique_ptr<StaticBuffer2D<Spectrum>> _frameBuffer;  //结果
};

namespace factory {
std::unique_ptr<IFactory> CreateAORenderer();  //环境光遮蔽渲染器
}
}  // namespace rad
