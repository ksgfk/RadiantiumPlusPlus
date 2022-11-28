#include <rad/realtime/dx12/render_context_dx12.h>

namespace Rad::DX12 {

static void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter) {
  *ppAdapter = nullptr;
  ComPtr<IDXGIAdapter1> adapter;
  ComPtr<IDXGIFactory6> factory6;
  if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
    for (
        uint32_t adapterIndex = 0;
        SUCCEEDED(factory6->EnumAdapterByGpuPreference(
            adapterIndex,
            requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
            IID_PPV_ARGS(&adapter)));
        ++adapterIndex) {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
        break;
      }
    }
  }
  if (adapter.Get() == nullptr) {
    for (uint32_t adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
        break;
      }
    }
  }
  *ppAdapter = adapter.Detach();
}

RenderContextDX12::RenderContextDX12(const Window& window, const RenderContextOptions& opts) : RenderContext() {
  if (opts.EnableDebug) {
    // DX12调试层
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
  }
  // DXGI处理与内核及系统硬件的交互
  // IDXGIFactory：DXGI工厂，用来创建其它的所有DXGI对象
  // IDXGIAdapter：适配器往往对应到一个显卡上，这个显卡可能是物理的也可能是软件模拟，或者是虚拟化的
  uint32_t dxgiFactoryFlags = 0;
  ComPtr<IDXGIFactory4> factory;
  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
  ComPtr<IDXGIAdapter1> adapter;
  GetHardwareAdapter(factory.Get(), adapter.GetAddressOf(), true);
  ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));

  ComPtr<ID3D12InfoQueue> infoQueue;
  if (SUCCEEDED(_device.As(&infoQueue))) {
    D3D12_MESSAGE_ID hide[] =
        {
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
    filter.DenyList.pIDList = hide;
    infoQueue->AddStorageFilterEntries(&filter);
  }

  //创建SwapChain需要一个Direct Queue用作执行命令队列，所以需要先创建
  // OpenGL里我们可以直接调用函数（是不是真的直接传给显卡我们不知道，反正对于调用者来说是的）将指令传给GPU，此时GPU必须不停的等待CPU的提交
  //但事实上CPU和GPU是异步进行的，这样提交效率极低
  //使用命令模式，创建一个CommandList将一连串指令全部记录下来，一次性提交给CommandQueue，
  // GPU再从CommandQueue取指令，此时两种设备异步执行，效率提升
  // 用于储存命令的是CommandAllocator，它和命令队列是分开的
  //当我们提交CommandList，GPU不一定会立刻去执行我们提交的命令，也可能正在处理之前的工作
  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_directCmdQueue)));
  ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_directCmdAlloc)));
  ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _directCmdAlloc.Get(), nullptr, IID_PPV_ARGS(&_directCmdList)));
  _directCmdList->Close();
  //显卡会将颜色根据我们的命令将结果画在一张纹理上，这是有个过程的，但是我们显然并不想看到画的过程而是直接看到绘制结果。
  //（展示画的过程会导致画面闪烁）
  //所以一般来说会让显卡把结果画在back buffer（后台缓冲区）上，当一帧绘制完毕后，再与front buffer（前台缓冲区）交换
  //交换后，下一帧再往当前的back buffer上绘制
  //这个过程称为Present
  // back buffer与front buffer一起构成了Swap Chain（交换链）
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.BufferCount = opts.SwapChainRTCount;
  swapChainDesc.Width = (UINT)opts.SwapChainRTSize.x();
  swapChainDesc.Height = (UINT)opts.SwapChainRTSize.y();
  swapChainDesc.Format = ConvertFormat(opts.SwapChainRTFormat);
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = opts.SwapChainMultiSample;
  ComPtr<IDXGISwapChain1> swapChain;
  ThrowIfFailed(factory->CreateSwapChainForHwnd(
      _directCmdQueue.Get(),
      (HWND)window.GetHandler(),
      &swapChainDesc,
      nullptr,
      nullptr,
      &swapChain));
  ThrowIfFailed(swapChain.As(&_swapChain));
  _nowFrameIndex = _swapChain->GetCurrentBackBufferIndex();
  _frameCount = swapChainDesc.BufferCount;
  _windowSize = window.GetSize();
  // DX12中，ID3D12Resource是一切资源的抽象，像是什么Buffer，Texture，Shader，Pipeline啥的都是资源
  // Buffer是最简单的GPU资源，它和std::vector<std::byte>没有什么不同
  //但是我们直接把这一堆字节交给显卡，显卡它不知道这堆是啥玩意，所以需要一个东西来描述，这一堆字节是啥
  // Descriptor做的就是这个工作，它就是std::span<T>，解释了一个资源是什么，在哪，有多少
  //与Descriptor关联最大的就是这个Descriptor Heap了，这又是个啥？
  //我们思考一下，好像描述一堆字节这件事情，本身就得使用一些空间，就算是std::span<T>里也要记录类型、内存地址、长度呢
  // Descriptor Heap就是用来储存这些描述信息的内存
  // Render Target是啥？如果说显卡绘制图像的行为是画画，那渲染目标（RT）就是承载画的画布
  //它基本上就是一张纹理，RT也是一种GPU资源
  //因此我们首先为所有RT的Descriptor开一小块空间
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
  rtvHeapDesc.NumDescriptors = _frameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ThrowIfFailed(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));
  _rtvDescSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  //接着为RT创建它的std::span，也就是Descriptor
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
  _rtv.resize(rtvHeapDesc.NumDescriptors);
  for (uint32_t n = 0; n < rtvHeapDesc.NumDescriptors; n++) {
    ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(&_rtv[n])));
    _device->CreateRenderTargetView(_rtv[n].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, _rtvDescSize);
  }
  //众所周知，RT还得至少带个depth buffer，最好还有个stencil buffer，这两个可以合并成一个buffer
  // TODO:

  ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
}

RenderContextDX12::RenderContextDX12(RenderContextDX12&& other) noexcept : RenderContext(std::move(other)) {
}

RenderContextDX12& RenderContextDX12::operator=(RenderContextDX12&& other) noexcept {
  RenderContext::operator=(std::move(other));
  return *this;
}

RenderContextDX12::~RenderContextDX12() noexcept {
}

DXGI_FORMAT RenderContextDX12::ConvertFormat(PixelFormat format) {
  switch (format) {
    case PixelFormat::RGBA32:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
      return DXGI_FORMAT_UNKNOWN;
  }
}

DXGI_FORMAT ConvertFormat(DepthStencilFormat format) {
  switch (format) {
    case DepthStencilFormat::R24G8:
      return DXGI_FORMAT_R24G8_TYPELESS;
    default:
      return DXGI_FORMAT_UNKNOWN;
  }
}

void RenderContextDX12::Draw() {
  ThrowIfFailed(_directCmdAlloc->Reset());
  ThrowIfFailed(_directCmdList->Reset(_directCmdAlloc.Get(), nullptr));

  D3D12_VIEWPORT viewport{};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = static_cast<float>(_windowSize.x());
  viewport.Height = static_cast<float>(_windowSize.y());
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  _directCmdList->RSSetViewports(1, &viewport);
  D3D12_RECT scissorRect{0, 0, static_cast<LONG>(_windowSize.x()), static_cast<LONG>(_windowSize.y())};
  _directCmdList->RSSetScissorRects(1, &scissorRect);

  auto rtvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      _rtv[_nowFrameIndex].Get(),
      D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  _directCmdList->ResourceBarrier(1, &rtvBarrier);

  auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      _rtvHeap->GetCPUDescriptorHandleForHeapStart(),
      _nowFrameIndex,
      _rtvDescSize);

  _directCmdList->OMSetRenderTargets(1, &rtv, false, nullptr);

  const FLOAT clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  _directCmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

  auto presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      _rtv[_nowFrameIndex].Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  _directCmdList->ResourceBarrier(1, &presentBarrier);

  ThrowIfFailed(_directCmdList->Close());

  ID3D12CommandList* ppCommandLists[] = {_directCmdList.Get()};
  _directCmdQueue->ExecuteCommandLists(1, ppCommandLists);

  ThrowIfFailed(_swapChain->Present(1, 0));

  _nowFrameIndex = (_nowFrameIndex + 1) % _rtv.size();

  _nowFence++;
  ThrowIfFailed(_directCmdQueue->Signal(_fence.Get(), _nowFence));
  if (_fence->GetCompletedValue() < _nowFence) {
    HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(_fence->SetEventOnCompletion(_nowFence, eventHandle));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

}  // namespace Rad
