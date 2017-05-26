#include "graphics\graphics.h"
#include "graphics-d3d12.h"

#include <cassert>

#include <d3d12.h>
#include <dxgi1_6.h>
#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif
#include <atlbase.h>

#define UNUSED(v) ((void)(v))

namespace {

template<typename T>
T GetProcAddressSafe(HMODULE hModule, char const* const lpProcName)
{
    if (hModule == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<T>(GetProcAddress(hModule, lpProcName));
}

template<class D>
void set_name(CComPtr<D> object, char const* format, ...)
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

template<typename I>
CComPtr<I> create_dxgi_debug_interface()
{
    auto dxgi_debugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgi_debugModule == nullptr) {
        return nullptr;
    }

    auto const pDXGIGetDebugInterface = GetProcAddressSafe<decltype(&DXGIGetDebugInterface)>(dxgi_debugModule, "DXGIGetDebugInterface");
    if (pDXGIGetDebugInterface == nullptr) {
        return nullptr;
    }
    CComPtr<I> dxgi_interface;
    HRESULT const hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&dxgi_interface));
    assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
    UNUSED(hr);
    return dxgi_interface;
}

} // anonymous namespace


namespace ak {

class GraphicsD3D12 : public Graphics
{
  public:
    GraphicsD3D12()
    {
        // Create objects
        create_factory();
    }
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
        _dxgi_debug = create_dxgi_debug_interface<IDXGIDebug1>();
        _dxgi_info_queue = create_dxgi_debug_interface<IDXGIInfoQueue>();
        if (_dxgi_debug) {
            _dxgi_debug->EnableLeakTrackingForThread();
        }
        if (_dxgi_info_queue) {
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, FALSE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
        }
#endif // _DEBUG
    }

    void create_factory()
    {
        constexpr UINT const factory_flags =
#if defined(_DEBUG)
            DXGI_CREATE_FACTORY_DEBUG |
#endif // _DEBUG
            0;
        HRESULT const hr = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&_factory));
        assert(SUCCEEDED(hr) && "Could not create factory");
        UNUSED(hr);
        set_name(_factory, "DXGI Factory");
    }

    //
    // Data members
    //

#if defined(_DEBUG)
    CComPtr<IDXGIDebug1>        _dxgi_debug;
    CComPtr<IDXGIInfoQueue>     _dxgi_info_queue;
    CComPtr<ID3D12InfoQueue>    d3d12InfoQueue;
#endif // _DEBUG
    CComPtr<IDXGIFactory4>  _factory;
};


ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

} // namespace ak
