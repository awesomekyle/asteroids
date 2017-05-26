#include "graphics\graphics.h"
#include "graphics-d3d12.h"

#include <cassert>

#include <d3d12.h>
#include <dxgi1_6.h>
#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif
#include <atlbase.h>

namespace {

template<typename T>
T GetProcAddressSafe(HMODULE hModule, char const* const lpProcName)
{
    if (hModule == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<T>(GetProcAddress(hModule, lpProcName));
}

template<typename I>
CComPtr<I> create_dxgi_debug_interface()
{
    auto dxgiDebugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgiDebugModule == nullptr) {
        return nullptr;
    }

    auto const pDXGIGetDebugInterface = GetProcAddressSafe<decltype(&DXGIGetDebugInterface)>(dxgiDebugModule, "DXGIGetDebugInterface");
    if (pDXGIGetDebugInterface == nullptr) {
        return nullptr;
    }
    CComPtr<I> dxgi_interface;
    HRESULT const hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&dxgi_interface));
    assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
    return dxgi_interface;
}

} // anonymous namespace


namespace ak {

class GraphicsD3D12 : public Graphics
{
  public:
    ~GraphicsD3D12()
    {

    }

    bool create_swap_chain(void* window, void* application) final {
        return false;
    }

    bool resize(int width, int height) final {
        return false;
    }

  private:
    void create_debug_interfaces()
    {
#if defined(_DEBUG)
        dxgiDebug = create_dxgi_debug_interface<IDXGIDebug1>();
        dxgiInfoQueue = create_dxgi_debug_interface<IDXGIInfoQueue>();
        if (dxgiDebug) {
            dxgiDebug->EnableLeakTrackingForThread();
        }
        if (dxgiInfoQueue) {
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, FALSE);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
        }
#endif // _DEBUG
    }

    //
    // Data members
    //
#if defined(_DEBUG)
    CComPtr<IDXGIDebug1>        dxgiDebug;
    CComPtr<IDXGIInfoQueue>     dxgiInfoQueue;
    CComPtr<ID3D12InfoQueue>    d3d12InfoQueue;
#endif // _DEBUG
};


ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

} // namespace ak
