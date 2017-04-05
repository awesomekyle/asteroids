extern "C" {
#include "gfx/gfx.h"
} // extern "C"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif

struct Gfx {
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

} // anonymous


extern "C" {

Gfx* gfxCreateD3D12(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));
    assert(G && "Could not allocate space for a D3D12 Gfx device");
    HRESULT hr = S_OK;
    hr = _CreateDebugInterfaces(G);
    return G;
}

void gfxDestroyD3D12(Gfx* G)
{
    assert(G);
    _SafeRelease(G->dxgi_info_queue);
    if (G->dxgi_debug) {
        G->dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
    _SafeRelease(G->dxgi_debug);
    free(G);
}

} // extern "C"
