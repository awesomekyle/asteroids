#include "graphics\graphics.h"
#include "graphics-d3d12.h"

#include <cassert>
#include <array>
#include <gsl/gsl_assert>

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
        create_debug_interfaces();
        create_factory();
        find_adapters();
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

        CComPtr<ID3D12Debug1> d3d12_debug;
        D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12_debug));
        if (d3d12_debug) {
            d3d12_debug->EnableDebugLayer();
            d3d12_debug->SetEnableGPUBasedValidation(TRUE);
            d3d12_debug->SetEnableSynchronizedCommandQueueValidation(TRUE);
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
        Ensures(_factory);
    }
    void find_adapters()
    {
        Expects(_factory);
        HRESULT hr = S_OK;
        for (size_t ii = 0; ii < _adapters.size(); ++ii) {
            auto& adapter = _adapters[ii];
            CComPtr<IDXGIAdapter1> adapter1 = nullptr;
            if (_factory->EnumAdapters1(static_cast<UINT>(ii), &adapter1) == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            CComPtr<IDXGIAdapter3> adapter3 = nullptr;
            hr = adapter1->QueryInterface(&adapter3);
            assert(SUCCEEDED(hr) && "Could not query for adapter3");
            adapter3->GetDesc2(&adapter.desc);
            if (adapter.desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                                           &adapter.memory_info);
            char buffer[128] = { '\0' };
            snprintf(buffer, sizeof(buffer), "%S", adapter.desc.Description);
            set_name(adapter3, buffer);

            // Set properties
            adapter.adapter = adapter3;
        }
    }

    //
    // Constants
    //
    static constexpr size_t kMaxAdapters = 4;

    //
    // Types
    //
    struct Adapter {
        CComPtr<IDXGIAdapter3>          adapter;
        DXGI_ADAPTER_DESC2              desc = {};
        DXGI_QUERY_VIDEO_MEMORY_INFO    memory_info = {};
    };

    //
    // Data members
    //
    CComPtr<IDXGIFactory4>  _factory;
    std::array<Adapter, kMaxAdapters> _adapters;

#if defined(_DEBUG)
    CComPtr<IDXGIDebug1>        _dxgi_debug;
    CComPtr<IDXGIInfoQueue>     _dxgi_info_queue;
    CComPtr<ID3D12InfoQueue>    d3d12InfoQueue;
#endif // _DEBUG
};


ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

} // namespace ak
