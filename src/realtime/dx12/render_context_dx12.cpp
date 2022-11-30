#include <rad/realtime/dx12/render_context_dx12.h>
/*
动画是一系列静态图像高速播放造成的效果。显卡的作用是绘制图像。如果直接往屏幕上绘制图像，绘制的过程会造成动画闪烁。
解决这个问题的办法是创建两个纹理，一个纹理在后台绘制，另一个纹理在屏幕上显示。
当后台绘制完成后，交换两张纹理，绘制完成的显示，另一张纹理到后台绘制。
这样前台缓冲区和后台缓冲区共同组成了交换链（Swap Chain）

DXGI是DirectX图形基础结构（DirectX Graphics Infrastructure）的缩写，它用于处理DX API与底层设备的交互
IDXGIFactory是DXGI工厂，用来创建其它的所有DXGI对象
IDXGIAdapter是适配器，往往对应到一个显示设备上，这个设备可以是物理的显卡的也可能是软件模拟，或者是虚拟化显卡

CPU与GPU是异步工作的两种设备，CPU会将指令放入一个命令队列中，GPU则从队列中取出来，这样就是一个生产者消费者模式

CPU首先使用CommandList暂时保存所有命令，接着在合适时机一次性提交给CommandQueue
实际储存命令的是CommandAllocator

既然是异步，肯定就会涉及到资源竞争问题。
假设CPU修改资源R，并提交绘制R的指令给GPU。由于提交指令后，CPU不会等待GPU绘制完成，因此CPU继续执行之后的任务
如果在GPU绘制前，CPU又修改了资源R，那么GPU绘制时就会出现问题
解决这个问题的办法是让CPU等待GPU完成资源R的绘制。这里需要用到栅栏（Fence），来强制CPU等待GPU完成到某个时间点

GPU内部也存在着大量异步行为，如果我们对资源R先写后读的操作，在GPU内实际上可能还没写，就已经开始读取了。这也会出现资源竞争
因此每种资源会设置一组状态，通过转换（Transition）状态来通知GPU对资源进行处理，来避免资源竞争。
在CPU端会使用Resource Barrier来通知GPU转换状态

DX12中，ID3D12Resource是一切资源的抽象，像是什么Buffer，Texture，Shader，Pipeline啥的都是资源
Buffer是最简单的GPU资源，它和std::vector<std::byte>没有什么不同
但是我们直接把这一堆字节交给显卡，显卡它不知道这堆是啥玩意，所以需要一个东西来描述，这一堆字节是啥
Descriptor做的就是这个工作，它就是std::span<T>，解释了一个资源是什么，在哪，有多少
与Descriptor关联最大的就是这个Descriptor Heap了，这又是个啥？
我们思考一下，好像描述一堆字节这件事情，本身就得使用一些空间，就算是std::span<T>里也要记录类型、内存地址、长度呢
Descriptor Heap就是用来储存这些描述信息的内存
Render Target是啥？如果说显卡绘制图像的行为是画画，那渲染目标（RT）就是承载画的画布
它基本上就是一张纹理，RT也是一种GPU资源
*/

namespace Rad::DX12 {

RenderContextDX12::RenderContextDX12(const Window& window, const RenderContextOptions& opts) {
  _logger = Logger::GetCategory("DX12");
  _hwnd = (HWND)window.GetHandler();
  _opts = opts;
  //创建device
  _device = std::make_unique<Device>(opts.EnableDebug, true, false);
  {
    auto adapterDesc = _device->GetAdapterDesc();
    auto videoMem = adapterDesc.DedicatedVideoMemory >> 20;
    char tmp[128];
    WideCharToMultiByte(CP_UTF8, 0, adapterDesc.Description, -1, tmp, 128, nullptr, nullptr);
    _logger->info("GPU: {} ({} MB)", tmp, videoMem);
  }
  //创建direct command queue
  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ThrowIfFailed(_device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));
  //创建swap chain
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.BufferCount = opts.SwapChainRTCount;
  swapChainDesc.Width = (UINT)opts.SwapChainRTSize.x();
  swapChainDesc.Height = (UINT)opts.SwapChainRTSize.y();
  swapChainDesc.Format = ConvertFormat(opts.SwapChainRTFormat);
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = opts.SwapChainMultiSample;
  ComPtr<IDXGISwapChain1> swapChain;
  ThrowIfFailed(_device->GetDXGIFactory()->CreateSwapChainForHwnd(
      _commandQueue.Get(),
      (HWND)window.GetHandler(),
      &swapChainDesc,
      nullptr,
      nullptr,
      &swapChain));
  ThrowIfFailed(swapChain.As(&_swapChain));
  _backBufferIndex = _swapChain->GetCurrentBackBufferIndex();
  //创建swap chain附带纹理的desc heap
  {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(_device->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));
    _rtvDescriptorSize = _device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(_device->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));
    _dsvDescriptorSize = _device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  }
  //创建帧资源
  {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    //为每一帧创建RTV和DSV
    _renderTargets.reserve(swapChainDesc.BufferCount);
    _depthTargets.reserve(swapChainDesc.BufferCount);
    for (UINT32 n = 0; n < swapChainDesc.BufferCount; n++) {
      _renderTargets.emplace_back(std::make_unique<Texture>(_device.get(), _swapChain.Get(), n));
      _depthTargets.emplace_back(std::make_unique<Texture>(
          _device.get(),
          (UINT)opts.SwapChainRTSize.x(),
          (UINT)opts.SwapChainRTSize.y(),
          ConvertFormat(opts.SwapChainDSFormat),
          TextureDimension::Tex2D,
          1,
          1,
          Texture::TextureUsage::DepthStencil,
          D3D12_RESOURCE_STATE_DEPTH_READ));
      _device->GetDevice()->CreateRenderTargetView(_renderTargets[n]->GetResource(), nullptr, rtvHandle);
      _device->GetDevice()->CreateDepthStencilView(_depthTargets[n]->GetResource(), nullptr, dsvHandle);
      rtvHandle.Offset(1, _rtvDescriptorSize);
      dsvHandle.Offset(1, _dsvDescriptorSize);
    }
    //为每一帧创建资源
    _frameResources.reserve(swapChainDesc.BufferCount);
    for (UINT32 n = 0; n < swapChainDesc.BufferCount; n++) {
      _frameResources.emplace_back(std::make_unique<FrameResource>(_device.get()));
    }
  }
  //创建fence
  {
    ThrowIfFailed(_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
    _fenceValue = 0;
  }
}

void RenderContextDX12::SyncCommandQueue() {
  _fenceValue++;
  _commandQueue->Signal(_fence.Get(), _fenceValue);
  if (_fence->GetCompletedValue() < _fenceValue) {
    LPCWSTR falseValue = 0;
    HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(_fence->SetEventOnCompletion(_fenceValue, eventHandle));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

void RenderContextDX12::Resize(const Vector2i& newSize) {
  if (newSize.x() <= 8 || newSize.y() <= 8) {
    return;
  }
  SyncCommandQueue();
  _renderTargets.clear();
  _depthTargets.clear();
  _frameResources.clear();
  _swapChain->ResizeBuffers(
      _opts.SwapChainRTCount,
      (UINT)newSize.x(), (UINT)newSize.y(),
      ConvertFormat(_opts.SwapChainRTFormat),
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
  _backBufferIndex = _swapChain->GetCurrentBackBufferIndex();
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
  CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT32 n = 0; n < _opts.SwapChainRTCount; n++) {
    _renderTargets.emplace_back(std::make_unique<Texture>(_device.get(), _swapChain.Get(), n));
    _depthTargets.emplace_back(std::make_unique<Texture>(
        _device.get(),
        (UINT)newSize.x(),
        (UINT)newSize.y(),
        ConvertFormat(_opts.SwapChainDSFormat),
        TextureDimension::Tex2D,
        1,
        1,
        Texture::TextureUsage::DepthStencil,
        D3D12_RESOURCE_STATE_DEPTH_READ));
    _device->GetDevice()->CreateRenderTargetView(_renderTargets[n]->GetResource(), nullptr, rtvHandle);
    _device->GetDevice()->CreateDepthStencilView(_depthTargets[n]->GetResource(), nullptr, dsvHandle);
    rtvHandle.Offset(1, _rtvDescriptorSize);
    dsvHandle.Offset(1, _dsvDescriptorSize);
  }
  _frameResources.reserve(_opts.SwapChainRTCount);
  for (UINT32 n = 0; n < _opts.SwapChainRTCount; n++) {
    _frameResources.emplace_back(std::make_unique<FrameResource>(_device.get()));
  }
  _opts.SwapChainRTSize = newSize;
}

void RenderContextDX12::Render() {
  const auto FrameCount = _opts.SwapChainRTCount;
  auto curFrame = _backBufferIndex;
  auto nextFrame = (curFrame + 1) % FrameCount;
  auto lastFrame = (nextFrame + 1) % FrameCount;
  //提交当前帧
  _frameResources[curFrame]->Execute(_commandQueue.Get(), _fence.Get(), _fenceValue);
  //呈现上一帧
  ThrowIfFailed(_swapChain->Present(0, 0));
  _backBufferIndex = (_backBufferIndex + 1) % FrameCount;
  //当前帧插个栅栏
  _frameResources[curFrame]->Signal(_commandQueue.Get(), _fence.Get());
  //提交下一帧
  PopulateCommandList(*_frameResources[nextFrame], nextFrame);
  _frameResources[lastFrame]->Sync(_fence.Get());
}

void RenderContextDX12::PopulateCommandList(FrameResource& frame, UINT index) {
  auto scoped = frame.Command();
  auto cmdList = scoped.GetCommandList();

  ResourceState state{};
  state.CollectState(_renderTargets[index].get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
  state.CollectState(_depthTargets[index].get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
  state.UpdateState(cmdList);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), index, _rtvDescriptorSize);
  CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), index, _dsvDescriptorSize);

  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  const FLOAT clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, Texture::CLEAR_DEPTH, Texture::CLEAR_STENCIL, 0, nullptr);

  state.RestoreState(cmdList);
}

DXGI_FORMAT RenderContextDX12::ConvertFormat(PixelFormat format) {
  switch (format) {
    case PixelFormat::RGBA32:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
      return DXGI_FORMAT_UNKNOWN;
  }
}

DXGI_FORMAT RenderContextDX12::ConvertFormat(DepthStencilFormat format) {
  switch (format) {
    case DepthStencilFormat::R24G8:
      return DXGI_FORMAT_R24G8_TYPELESS;
    case DepthStencilFormat::Depth32Float:
      return DXGI_FORMAT_D32_FLOAT;
    default:
      return DXGI_FORMAT_UNKNOWN;
  }
}

}  // namespace Rad::DX12
