extern "C" {
#include "gfx/gfx.h"
#include "gfx-d3d12.h"
} // extern "C"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <atomic>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#if defined(_DEBUG)
#   include <dxgidebug.h>
#endif
#include <atlbase.h>

#pragma warning(push)
#pragma warning(disable:4324) // structure was padded due to alignment specifier
#include <d3dx12.h>
#pragma warning(pop)

#include "../gfx-internal.h"
#include "gfx-d3d12-helper.h"

static constexpr UINT kFramesInProgress = 3;
static constexpr size_t kMaxBackBuffers = 4;
static constexpr size_t kMaxCommandLists = 128;

struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
    Gfx* G;
    ID3D12CommandAllocator*     allocator;
    ID3D12GraphicsCommandList*  list;
    uint64_t    completion;
};

struct GfxRenderState {
    GfxRenderStateDesc      desc;
    ID3D12RootSignature*    rootSignature;
    ID3D12PipelineState*    pso;
};

struct Gfx {
    GfxTable const* table;
    // Types
    struct Adapter {
        IDXGIAdapter3*      adapter;
        DXGI_ADAPTER_DESC2  desc;
        DXGI_QUERY_VIDEO_MEMORY_INFO    memory_info;
    };
    struct DescriptorHeap {
        ID3D12DescriptorHeap*       heap;
        D3D12_DESCRIPTOR_HEAP_DESC  desc;
        D3D12_DESCRIPTOR_HEAP_TYPE  type;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStart;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStart;
        UINT handleSize;

        inline D3D12_CPU_DESCRIPTOR_HANDLE CpuSlot(int slot)
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, slot, handleSize);
        }
        inline D3D12_GPU_DESCRIPTOR_HANDLE GpuSlot(int slot)
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, slot, handleSize);
        }
        operator ID3D12DescriptorHeap* ()
        {
            return heap;
        }
    };

    IDXGIFactory4*  factory;
    Adapter         adapters[4];
    Adapter*        currentAdapter;

    ID3D12Device*       device;
    D3D_FEATURE_LEVEL   featureLevel;
    ID3D12CommandQueue* renderQueue;
    ID3D12Fence*        renderFence;
    uint64_t            lastFenceCompletion;

    IDXGISwapChain3*        swapChain;
    DXGI_SWAP_CHAIN_DESC1   swapChainDesc;
    ID3D12Resource*         backBuffers[kMaxBackBuffers];

    DescriptorHeap  rtvHeap;

    // command lists
    GfxCmdBuffer commandLists[kMaxCommandLists];
    std::atomic<uint32_t>   currentCommandList;

#if defined(_DEBUG)
    IDXGIDebug1*        dxgiDebug;
    IDXGIInfoQueue*     dxgiInfoQueue;
    ID3D12InfoQueue*    d3d12InfoQueue;
#endif
};

//////
// Helper methods
//////
namespace {

constexpr D3D12_CULL_MODE D3D12CullMode(GfxCullMode cullMode)
{
    switch (cullMode) {
        case kFront:
            return D3D12_CULL_MODE_FRONT;
        case kBack:
            return D3D12_CULL_MODE_BACK;
        case kNone:
            return D3D12_CULL_MODE_NONE;
        default:
            break;
    }
    return D3D12_CULL_MODE_NONE;
};

template<class D>
void _SetName(D* object, char const* format, ...)
{
    va_list args;
    if (object && format) {
        char buffer[128] = {};
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strnlen(buffer, sizeof(buffer)), buffer);
    }
}

template<class D>
ULONG _SafeRelease(D*& obj)
{
    ULONG count = 0;
    if (obj) {
        count = obj->Release();
        obj = nullptr;
    }
    return count;
}

HRESULT _CreateDebugInterfaces(Gfx* const G)
{
    assert(G);
    HRESULT hr = S_OK;
    (void)G;
#if defined(_DEBUG)
    // create debug interfaces
    auto const dxgiDebugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgiDebugModule) {
        decltype(&DXGIGetDebugInterface) const pDXGIGetDebugInterface = (decltype(&DXGIGetDebugInterface))(GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface"));
        if (pDXGIGetDebugInterface) {
            hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&G->dxgiDebug));
            assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
            hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&G->dxgiInfoQueue));
            assert(SUCCEEDED(hr) && "Could not create DXGIInfoQueue interface");
            if (G->dxgiDebug) {
                G->dxgiDebug->EnableLeakTrackingForThread();
            }
            if (G->dxgiInfoQueue) {
                G->dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
                G->dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                G->dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
                G->dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, FALSE);
                G->dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
            }
        }
    }
#endif
    return hr;
}
HRESULT _CreateFactory(Gfx* const G)
{
    assert(G);
    auto const dxgiModule = LoadLibraryA("dxgi.dll");
    assert(dxgiModule && "Could not load dxgi.dll");
    decltype(&CreateDXGIFactory2) const pCreateDXGIFactory2 = (decltype(&CreateDXGIFactory2))(GetProcAddress(dxgiModule, "CreateDXGIFactory2"));
    assert(pCreateDXGIFactory2 && "Could not load CreateDXGIFactory2 address");
    UINT const factoryFlags =
#if defined(_DEBUG)
        DXGI_CREATE_FACTORY_DEBUG |
#endif
        0;
    HRESULT const hr = pCreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&G->factory));
    _SetName(G->factory, "DXGI Factory");
    return hr;
}
HRESULT _FindAdapters(Gfx* const G)
{
    assert(G);
    HRESULT hr = S_OK;
    for (UINT ii = 0; ii < _countof(G->adapters); ++ii) {
        auto& adapter = G->adapters[ii];
        CComPtr<IDXGIAdapter1> adapter1 = nullptr;
        if (G->factory->EnumAdapters1(ii, &adapter1) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        CComPtr<IDXGIAdapter3> adapter3 = nullptr;
        hr = adapter1->QueryInterface(&adapter3);
        assert(SUCCEEDED(hr) && "Could not query for adapter3");
        DXGI_ADAPTER_DESC2 desc2 = { 0 };
        adapter3->GetDesc2(&desc2);
        if (desc2.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        adapter.adapter = adapter3.Detach();
        adapter.desc = desc2;
        adapter.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                                              &adapter.memory_info);
        char buffer[128] = { '\0' };
        snprintf(buffer, sizeof(buffer), "%S", adapter.desc.Description);
        _SetName(adapter.adapter, buffer);
    }
    return hr;
}
HRESULT _CreateDevice(Gfx* const G)
{
    assert(G);
    auto const d3d12Module = LoadLibraryA("d3d12.dll");
    assert(d3d12Module && "Could not load d3d12.dll");
#if defined(_DEBUG)
    // create debug interface
    decltype(&D3D12GetDebugInterface) const pD3D12GetDebugInterface = decltype(&D3D12GetDebugInterface)(GetProcAddress(d3d12Module, "D3D12GetDebugInterface"));
    CComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(pD3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    HRESULT hr = S_OK;
    decltype(&D3D12CreateDevice) const pD3D12CreateDevice = decltype(&D3D12CreateDevice)(GetProcAddress(d3d12Module, "D3D12CreateDevice"));
    for (auto& adapter : G->adapters) {
        if (adapter.adapter == nullptr) {
            break;
        }
        hr = pD3D12CreateDevice(adapter.adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&G->device));
        if (SUCCEEDED(hr)) {
            D3D_FEATURE_LEVEL const levels[] = {
                D3D_FEATURE_LEVEL_9_1,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_12_1
            };
            D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {_countof(levels), levels};
            hr = G->device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
            assert(SUCCEEDED(hr));
            G->featureLevel = featureLevels.MaxSupportedFeatureLevel;
            G->currentAdapter = &adapter;
            G->device->SetStablePowerState(TRUE);
            _SetName(G->device, "D3D12 device");
            break;
        }
    }

#if defined(_DEBUG)
    G->device->QueryInterface(IID_PPV_ARGS(&G->d3d12InfoQueue));
    if (G->d3d12InfoQueue) {
        G->d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        G->d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        G->d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
        G->d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, FALSE);
        G->d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE, FALSE);
    }
#endif
    return hr;
}
HRESULT _CreateQueues(Gfx* const G)
{
    assert(G);
    D3D12_COMMAND_QUEUE_DESC const queueDesc = {
        D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
        0,
        D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
        0
    };
    HRESULT hr = G->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&G->renderQueue));
    assert(SUCCEEDED(hr) && "Could not create queue");
    _SetName(G->renderQueue, "Render Queue");
    hr = G->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&G->renderFence));
    assert(SUCCEEDED(hr) && "Could not create render fence");
    _SetName(G->renderFence, "Render Fence");
    return hr;
}
void _WaitForIdle(Gfx* const G)
{
    assert(G);
    G->lastFenceCompletion++;
    G->renderQueue->Signal(G->renderFence, G->lastFenceCompletion);
    while (G->renderFence->GetCompletedValue() < G->lastFenceCompletion)
        ;
}

Gfx::DescriptorHeap CreateDescriptorHeap(ID3D12Device* device,
                                         D3D12_DESCRIPTOR_HEAP_DESC const& desc,
                                         char const* name = nullptr)
{
    Gfx::DescriptorHeap heap = {};
    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.heap));
    assert(SUCCEEDED(hr));
    _SetName(heap.heap, name);

    heap.desc = desc;
    heap.type = desc.Type;
    heap.handleSize = device->GetDescriptorHandleIncrementSize(desc.Type);
    heap.cpuStart = heap.heap->GetCPUDescriptorHandleForHeapStart();
    if (desc.Flags | D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        heap.gpuStart = heap.heap->GetGPUDescriptorHandleForHeapStart();
    }
    return heap;
}


} // anonymous


Gfx* gfxD3D12Create(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));

    assert(G && "Could not allocate space for a D3D12 Gfx device");
    HRESULT hr = S_OK;

    hr = _CreateDebugInterfaces(G);
    assert(SUCCEEDED(hr));
    hr = _CreateFactory(G);
    assert(SUCCEEDED(hr));
    hr = _FindAdapters(G);
    assert(SUCCEEDED(hr));
    hr = _CreateDevice(G);
    assert(SUCCEEDED(hr));
    hr = _CreateQueues(G);
    assert(SUCCEEDED(hr));

    // Descriptor heaps
    D3D12_DESCRIPTOR_HEAP_DESC const rtvHeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0
    };
    G->rtvHeap = CreateDescriptorHeap(G->device, rtvHeapDesc, "RTV Heap");

    // Command lists
    for (size_t ii = 0; ii < kMaxCommandLists; ++ii) {
        auto& commandList = G->commandLists[ii];
        commandList.G = G;
        hr = G->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&commandList.allocator));
        assert(SUCCEEDED(hr) && "Could not create allocator");
        hr = G->device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          commandList.allocator, nullptr,
                                          IID_PPV_ARGS(&commandList.list));
        assert(SUCCEEDED(hr) && "Could not create allocator");
        hr = commandList.list->Close();
        assert(SUCCEEDED(hr) && "Could not close empty command list");
        _SetName(commandList.allocator, "Command Allocator %zu", ii);
        _SetName(commandList.list, "Command list %zu", ii);
        commandList.table = &GfxD3D12CmdBufferTable;
    }

    G->table = &GfxD3D12Table;

    return G;
}

void gfxD3D12Destroy(Gfx* G)
{
    assert(G);
    _WaitForIdle(G);
    for (auto& commandList : G->commandLists) {
        _SafeRelease(commandList.allocator);
        _SafeRelease(commandList.list);
    }
    _SafeRelease(G->rtvHeap.heap);
    for (auto*& backBuffer : G->backBuffers) {
        _SafeRelease(backBuffer);
    }
    _SafeRelease(G->swapChain);
    _SafeRelease(G->renderFence);
    _SafeRelease(G->renderQueue);
    _SafeRelease(G->device);
    for (auto& adapter : G->adapters) {
        _SafeRelease(adapter.adapter);
    }
    _SafeRelease(G->factory);
#if _DEBUG
    _SafeRelease(G->d3d12InfoQueue);
    _SafeRelease(G->dxgiInfoQueue);
    if (G->dxgiDebug) {
        G->dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
    _SafeRelease(G->dxgiDebug);
#endif // _DEBUG
    free(G);
}

bool gfxD3D12CreateSwapChain(Gfx* const G, void* const window, void* const application)
{
    assert(G);
    (void)application;
    if (window == nullptr) {
        return false;
    }
    HWND hwnd = static_cast<HWND>(window);
    DXGI_SWAP_CHAIN_DESC1 const swapChainDesc = {
        0, // Width
        0, // Height
        DXGI_FORMAT_B8G8R8A8_UNORM, // Format
        FALSE, // Stereo
        {
            1, // Count
            0, // Quality
        }, // SampleDesc
        DXGI_USAGE_RENDER_TARGET_OUTPUT, // BufferUsage
        kFramesInProgress, // BufferCount
        DXGI_SCALING_NONE, // Scaling
        DXGI_SWAP_EFFECT_FLIP_DISCARD, // SwapEffect
        DXGI_ALPHA_MODE_UNSPECIFIED, // AlphaMode
        0, // Flags
    };
    CComPtr<IDXGISwapChain1> swapchain1 = nullptr;
    HRESULT hr = G->factory->CreateSwapChainForHwnd(G->renderQueue,
                                                    hwnd,
                                                    &swapChainDesc,
                                                    nullptr,
                                                    nullptr,
                                                    &swapchain1);
    assert(SUCCEEDED(hr) && "Could not create swap chain");
    hr = swapchain1->QueryInterface(IID_PPV_ARGS(&G->swapChain));
    assert(SUCCEEDED(hr) && "Could not get swapchain3");
    hr = G->factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(hr) && "Could not disable alt-enter");

    _SetName(G->swapChain, "DXGI Swap Chain");
    G->swapChainDesc = swapChainDesc;

    gfxD3D12Resize(G, G->swapChainDesc.Width, G->swapChainDesc.Height);

    return true;
}
bool gfxD3D12Resize(Gfx* const G, int const /*width*/, int const /*height*/)
{
    assert(G);
    if (G->swapChain == nullptr) {
        return false;
    }
    _WaitForIdle(G);
    for (auto*& backBuffer : G->backBuffers) {
        _SafeRelease(backBuffer);
    }
    HRESULT hr = G->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    assert(SUCCEEDED(hr) && "Could not resize buffers");
    G->swapChain->GetDesc1(&G->swapChainDesc);
    for (UINT ii = 0; ii < G->swapChainDesc.BufferCount; ++ii) {
        hr = G->swapChain->GetBuffer(ii, IID_PPV_ARGS(&G->backBuffers[ii]));
        assert(SUCCEEDED(hr) && "Could not get back buffer");
        _SetName(G->backBuffers[ii], "Back Buffer %d", ii);
        auto const backBufferSlot = G->rtvHeap.CpuSlot(ii);
        G->device->CreateRenderTargetView(G->backBuffers[ii], nullptr, backBufferSlot);
    }
    // transition current back buffer to render target
    UINT const currentIndex = G->swapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* const currentBackBuffer = G->backBuffers[currentIndex];
    auto const barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer,
                                                              D3D12_RESOURCE_STATE_PRESENT,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET);

    // make back buffer presentable
    GfxCmdBuffer* const commandList = gfxD3D12GetCommandBuffer(G);
    commandList->list->ResourceBarrier(1, &barrier);
    gfxD3D12ExecuteCommandBuffer(commandList);
    return true;
}
GfxRenderTarget gfxD3D12GetBackBuffer(Gfx* G)
{
    assert(G);
    UINT const frameIndex = G->swapChain->GetCurrentBackBufferIndex();
    return G->rtvHeap.CpuSlot(frameIndex).ptr;
}
bool gfxD3D12Present(Gfx* G)
{
    assert(G);
    auto const currentIndex = G->swapChain->GetCurrentBackBufferIndex();
    auto const nextIndex = (currentIndex + 1) % kFramesInProgress;

    // set up barriers
    D3D12_RESOURCE_BARRIER const barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(G->backBuffers[currentIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
        CD3DX12_RESOURCE_BARRIER::Transition(G->backBuffers[nextIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
    };

    // transition to present
    GfxCmdBuffer* commandList = gfxD3D12GetCommandBuffer(G);
    assert(commandList);
    commandList->list->ResourceBarrier(1, barriers + 0);
    bool result = gfxD3D12ExecuteCommandBuffer(commandList);
    assert(result);

    if (G->swapChain) {
        DXGI_PRESENT_PARAMETERS const params = {
            0,
            nullptr,
            nullptr,
            nullptr,
        };
        HRESULT const hr = G->swapChain->Present1(0, 0, &params);
        assert(SUCCEEDED(hr));
    }

    // transition previous back buffer
    commandList = gfxD3D12GetCommandBuffer(G);
    assert(commandList);
    commandList->list->ResourceBarrier(1, barriers + 1);
    result = gfxD3D12ExecuteCommandBuffer(commandList);
    assert(result);
    return true;
}
GfxRenderState* gfxD3D12CreateRenderState(Gfx* G, GfxRenderStateDesc const* desc)
{
    assert(G);
    assert(desc);
    HRESULT hr = S_OK;
    ID3D12PipelineState* pso = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;

    // input layout
    auto const* layout = desc->layout;
    D3D12_INPUT_ELEMENT_DESC inputLayout[16] = { { nullptr } };
    uint32_t numInputElements = 0;
    UINT current_offset = 0;
    while (layout && layout->name) {
        inputLayout[numInputElements++] = {
            layout->name, 0, DXGIFormatFromGfxFormat(layout->format), 0, current_offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        };
        UINT const offset = (UINT)gfxFormatSize(layout->format);
        current_offset += offset;
        layout++;
    }
    // compile shaders
    ID3DBlob* vsBytecode = nullptr;
    hr = CompileHLSLShader(desc->vertexShader.source,
                           "vs_5_1",
                           desc->vertexShader.entrypoint,
                           &vsBytecode);
    assert(vsBytecode && "Shader compilation failed");
    assert(SUCCEEDED(hr) && "Could not compile shader");
    ID3DBlob* psBytecode = nullptr;
    hr = CompileHLSLShader(desc->pixelShader.source,
                           "ps_5_1",
                           desc->pixelShader.entrypoint,
                           &psBytecode);
    assert(psBytecode && "Shader compilation failed");
    assert(SUCCEEDED(hr) && "Could not compile shader");

    // root signatures
    CD3DX12_DESCRIPTOR_RANGE srvRanges[4];
    srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    srvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    srvRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    srvRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    CD3DX12_ROOT_PARAMETER rootParameters[8];
    // 4 single CBVs, 4 single SRVs
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[4].InitAsDescriptorTable(1, srvRanges + 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, srvRanges + 1, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[6].InitAsDescriptorTable(1, srvRanges + 2, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[7].InitAsDescriptorTable(1, srvRanges + 3, D3D12_SHADER_VISIBILITY_PIXEL);
    // single static sampler
    D3D12_STATIC_SAMPLER_DESC const samplerDesc = {
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        16,
        D3D12_COMPARISON_FUNC_ALWAYS,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
        0.0f,
        D3D12_FLOAT32_MAX,
        0,
        0,
        D3D12_SHADER_VISIBILITY_PIXEL,
    };
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters,
                           1, &samplerDesc,
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    CComPtr<ID3DBlob> signature;
    CComPtr<ID3DBlob> error;
    auto const pD3D12 = LoadLibraryA("d3d12.dll");
    assert(pD3D12 && "Could not load D3D12...how are we here?");
    auto const pD3D12SerializeRootSignature = decltype(&D3D12SerializeRootSignature)(GetProcAddress(pD3D12, "D3D12SerializeRootSignature"));
    assert(pD3D12SerializeRootSignature && "Could not find D3D12SerializeRootSignature");
    hr = pD3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (error) {
        fprintf(stderr, "Eror serializing root signature: %s", static_cast<char const*>(error->GetBufferPointer()));
    }
    assert(SUCCEEDED(hr));
    hr = G->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                        IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    // PSO
    CD3DX12_DEPTH_STENCIL_DESC depthDesc(D3D12_DEFAULT);
    depthDesc.DepthWriteMask = desc->depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    CD3DX12_RASTERIZER_DESC const rsDesc(D3D12_FILL_MODE_SOLID,
                                         D3D12CullMode(desc->culling),
                                         FALSE, 0, 0.0f, 0.0f, TRUE, TRUE,
                                         FALSE, FALSE, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
    D3D12_BLEND_DESC const blendDesc = {
        FALSE, FALSE,
        {
            {
                TRUE,
                FALSE,
                D3D12_BLEND_SRC_ALPHA,
                D3D12_BLEND_INV_SRC_ALPHA,
                D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE,
                D3D12_BLEND_ZERO,
                D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL,
            },
        },
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC const psoDesc = {
        rootSignature, // pRootSignature
        CD3DX12_SHADER_BYTECODE(vsBytecode), // VS
        CD3DX12_SHADER_BYTECODE(psBytecode), // PS
        { nullptr, 0 }, // DS
        { nullptr, 0 }, // HS
        { nullptr, 0 }, // GS
        {
            // StreamOutput
            nullptr,    // pSODeclaration
            0,          // NumEntries
            nullptr,    // pBufferStrides
            0,          // NumStrides
            0,          // RasterizedStream
        },
        blendDesc, // BlendState
        UINT_MAX, //  SampleMask
        rsDesc, // RasterizerState
        depthDesc, // DepthStencilState
        { inputLayout, numInputElements }, // InputLayout
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, // IBStripCutValue
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // PrimitiveTopologyType
        1, // NumRenderTargets
        { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB }, // RTVFormats TODO: Parameterize
        { DXGIFormatFromGfxFormat(desc->depthFormat) }, // DSVFormat
        { 1, 0 }, // SampleDesc
        0, // NodeMask
        { nullptr, 0 }, // CachedPSO
        D3D12_PIPELINE_STATE_FLAG_NONE, // Flags
    };
    hr = G->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr) && "Could not create PSO");
    _SetName(pso, desc->name);

    psBytecode->Release();
    vsBytecode->Release();

    GfxRenderState* const state = (GfxRenderState*)calloc(1, sizeof(*state));
    state->desc = *desc;
    state->pso = pso;
    state->rootSignature = rootSignature;
    return state;
}
void gfxD3D12DestroyRenderState(Gfx* G, GfxRenderState* state)
{
    assert(G);
    if (state) {
        _SafeRelease(state->pso);
        _SafeRelease(state->rootSignature);
    }
    free(state);
}
GfxCmdBuffer* gfxD3D12GetCommandBuffer(Gfx* G)
{
    assert(G);
    uint_fast32_t const currIndex = G->currentCommandList.fetch_add(1) % kMaxCommandLists;
    GfxCmdBuffer* const list = &G->commandLists[currIndex];
    uint64_t const lastCompletedValue = G->renderFence->GetCompletedValue();
    if (lastCompletedValue < list->completion) {
        /// @TODO: This case needs to be handled
        //assert(lastCompletedValue >= list->completion && "Oldest command buffer not yet completed. Continue through the queue?");
        return nullptr;
    }
    list->completion = UINT64_MAX;
    HRESULT hr = list->allocator->Reset();
    assert(SUCCEEDED(hr) && "Could not reset allocator");
    hr = list->list->Reset(list->allocator, nullptr);
    assert(SUCCEEDED(hr) && "Could not reset command list");
    return list;
}
int gfxD3D12NumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    int freeBuffers = 0;
    for (uint32_t ii = 0; ii < kMaxCommandLists; ++ii) {
        uint32_t const index = G->currentCommandList.fetch_add(1) & (kMaxCommandLists - 1);
        auto& commandList = G->commandLists[index];
        if (G->renderFence->GetCompletedValue() >= commandList.completion) {
            freeBuffers++;
        }
    }
    return freeBuffers;
}
void gfxD3D12ResetCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    HRESULT hr = B->list->Close();
    assert(SUCCEEDED(hr) && "Could not close command list");
    B->completion = 0;
}

bool gfxD3D12ExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    Gfx* const G = B->G;
    HRESULT const hr = B->list->Close();
    assert(SUCCEEDED(hr) && "Could not close command list");
    G->renderQueue->ExecuteCommandLists(1, CommandListCast(&B->list));
    B->completion = G->lastFenceCompletion++;
    G->renderQueue->Signal(G->renderFence, B->completion);
    return SUCCEEDED(hr);
}

void gfxD3D12CmdBeginRenderPass(GfxCmdBuffer* B,
                                GfxRenderTarget renderTargetHandle,
                                GfxRenderPassAction loadAction,
                                float const clearColor[4])
{
    assert(B);
    D3D12_CPU_DESCRIPTOR_HANDLE const rtDescriptor = { (SIZE_T)renderTargetHandle };
    if (renderTargetHandle != kGfxInvalidHandle) {
        B->list->OMSetRenderTargets(1, &rtDescriptor, false, nullptr);
        if (loadAction == kGfxRenderPassActionClear) {
            B->list->ClearRenderTargetView(rtDescriptor, clearColor, 0, nullptr);
        }
    }
}

void gfxD3D12CmdEndRenderPass(GfxCmdBuffer* /*B*/)
{
}


GfxTable const GfxD3D12Table = {
    gfxD3D12Destroy,
    gfxD3D12CreateSwapChain,
    gfxD3D12Resize,
    gfxD3D12GetBackBuffer,
    gfxD3D12Present,
    gfxD3D12CreateRenderState,
    gfxD3D12DestroyRenderState,
    gfxD3D12GetCommandBuffer,
    gfxD3D12NumAvailableCommandBuffers,
};
GfxCmdBufferTable const GfxD3D12CmdBufferTable = {
    gfxD3D12ResetCommandBuffer,
    gfxD3D12ExecuteCommandBuffer,
    gfxD3D12CmdBeginRenderPass,
    gfxD3D12CmdEndRenderPass,
};

