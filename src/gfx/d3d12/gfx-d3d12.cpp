extern "C" {
#include "gfx/gfx.h"
} // extern "C"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif
#include <atlbase.h>

struct Gfx {
    // Types
    struct Adapter {
        IDXGIAdapter3*      adapter;
        DXGI_ADAPTER_DESC2  desc;
        DXGI_QUERY_VIDEO_MEMORY_INFO    memory_info;
    };

    IDXGIFactory4*  factory;
    Adapter         adapters[4];
    Adapter*        currentAdapter;

    ID3D12Device*       device;
    D3D_FEATURE_LEVEL   featureLevel;
    ID3D12CommandQueue* renderQueue;
    ID3D12Fence*        renderFence;

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

template<class D>
void _SetName(D* const object, char const* const name)
{
    if (name) {
        object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strnlen(name, 128), name);
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
    HRESULT hr = S_OK;
    (void)G;
#if defined(_DEBUG)
    // create debug interfaces
    auto const dxgiDebugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgiDebugModule) {
        auto const* const pDXGIGetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface"));
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
    auto const dxgiModule = LoadLibraryA("dxgi.dll");
    assert(dxgiModule && "Could not load dxgi.dll");
    auto const* pCreateDXGIFactory2 = reinterpret_cast<decltype(&CreateDXGIFactory2)>(GetProcAddress(dxgiModule, "CreateDXGIFactory2"));
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
    auto const d3d12Module = LoadLibraryA("d3d12.dll");
    assert(d3d12Module && "Could not load d3d12.dll");
#if defined(_DEBUG)
    // create debug interface
    auto const* pD3D12GetDebugInterface = decltype(&D3D12GetDebugInterface)(GetProcAddress(d3d12Module, "D3D12GetDebugInterface"));
    CComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(pD3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    HRESULT hr = S_OK;
    auto const* pD3D12CreateDevice = decltype(&D3D12CreateDevice)(GetProcAddress(d3d12Module, "D3D12CreateDevice"));
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

} // anonymous


extern "C" {

Gfx* gfxCreateD3D12(void)
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

    return G;
}

void gfxDestroyD3D12(Gfx* G)
{
    assert(G);
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

bool gfxCreateSwapChain(Gfx* const G, void* const window)
{
    UNREFERENCED_PARAMETER(G);
    UNREFERENCED_PARAMETER(window);
    return true;
}

} // extern "C"
