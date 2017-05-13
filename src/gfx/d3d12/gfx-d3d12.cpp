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

#if defined(_DEBUG)
    IDXGIDebug1*    dxgi_debug;
    IDXGIInfoQueue* dxgi_info_queue;
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
            hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&G->dxgi_debug));
            assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
            hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&G->dxgi_info_queue));
            assert(SUCCEEDED(hr) && "Could not create DXGIInfoQueue interface");
            if (G->dxgi_debug) {
                G->dxgi_debug->EnableLeakTrackingForThread();
            }
            if (G->dxgi_info_queue) {
                G->dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
                G->dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                G->dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
                G->dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, FALSE);
                G->dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
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
    return G;
}

void gfxDestroyD3D12(Gfx* G)
{
    assert(G);
    for (auto& adapter : G->adapters) {
        _SafeRelease(adapter.adapter);
    }
    _SafeRelease(G->factory);
    #if _DEBUG
        _SafeRelease(G->dxgi_info_queue);
        if (G->dxgi_debug) {
            G->dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
        _SafeRelease(G->dxgi_debug);
    #endif // _DEBUG
    free(G);
}

} // extern "C"
