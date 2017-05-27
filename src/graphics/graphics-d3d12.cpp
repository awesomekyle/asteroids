#include "graphics-d3d12.h"
#include "graphics/graphics.h"

#include <array>
#include <cassert>
#include <gsl/gsl_assert>

#include <d3d12.h>
#include <dxgi1_6.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <atlbase.h>

#include <d3dx12.h>
#include <gsl/gsl>

#define UNUSED(v) ((void)(v))

namespace {

struct DescriptorHeap
{
    CComPtr<ID3D12DescriptorHeap> heap = {};
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    D3D12_DESCRIPTOR_HEAP_TYPE type = {};
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_start = {};
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_start = {};
    UINT handleSize = {};

    inline D3D12_CPU_DESCRIPTOR_HANDLE cpu_slot(int slot)
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_start, slot, handleSize);
    }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GpuSlot(int slot)
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_start, slot, handleSize);
    }
    explicit operator ID3D12DescriptorHeap*() { return heap; }
};

template<typename T>
T GetProcAddressSafe(HMODULE hModule, char const* const lpProcName)  // NOLINT
{
    if (hModule == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<T>(GetProcAddress(hModule, lpProcName));
}

template<class D, typename... Args>
void set_name(CComPtr<D> object, char const* format, Args const&... args)
{
    if (object && format) {
        char buffer[128] = {};
        snprintf(buffer, sizeof(buffer), format, args...);  // NOLINT
        object->SetPrivateData(WKPDID_D3DDebugObjectName,
                               (UINT)strnlen(buffer, sizeof(buffer)),  // NOLINT
                               buffer);                                // NOLINT
    }
}

template<typename I>
CComPtr<I> create_dxgi_debug_interface()
{
    auto dxgi_debugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgi_debugModule == nullptr) {
        return nullptr;
    }

    auto const pDXGIGetDebugInterface = GetProcAddressSafe<decltype(&DXGIGetDebugInterface)>(
        dxgi_debugModule, "DXGIGetDebugInterface");
    if (pDXGIGetDebugInterface == nullptr) {
        return nullptr;
    }
    CComPtr<I> dxgi_interface;
    HRESULT const hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&dxgi_interface));
    assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
    UNUSED(hr);
    return dxgi_interface;
}

DescriptorHeap CreateDescriptorHeap(ID3D12Device* const device,
                                    D3D12_DESCRIPTOR_HEAP_DESC const& desc,
                                    char const* const name = nullptr)
{
    Expects(device);
    DescriptorHeap heap = {};
    HRESULT const hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.heap));
    assert(SUCCEEDED(hr));
    UNUSED(hr);
    set_name(heap.heap, name);

    heap.desc = desc;
    heap.type = desc.Type;
    heap.handleSize = device->GetDescriptorHandleIncrementSize(desc.Type);
    heap.cpu_start = heap.heap->GetCPUDescriptorHandleForHeapStart();
    if ((desc.Flags | D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0) {
        heap.gpu_start = heap.heap->GetGPUDescriptorHandleForHeapStart();
    }
    return heap;
}
bool developer_mode_enabled()
{
    HKEY hKey = {};
    auto err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,  // NOLINT
                             LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock)", 0,
                             KEY_READ, &hKey);
    if (err != ERROR_SUCCESS) {
        return false;
    }
    DWORD value = {};
    DWORD dword_size = sizeof(DWORD);
    err = RegQueryValueExW(hKey, L"AllowDevelopmentWithoutDevLicense", nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&value), &dword_size);  // NOLINT
    RegCloseKey(hKey);
    if (err != ERROR_SUCCESS) {
        return false;
    }
    return value != 0;
}

}  // anonymous namespace

namespace ak {

class GraphicsD3D12 : public Graphics
{
   public:
    GraphicsD3D12()
        : _device(nullptr)
        , _render_queue(nullptr)
    {
        create_debug_interfaces();
        create_factory();
        find_adapters();
        create_device();
        create_queue();
        create_descriptor_heaps();
    }
    ~GraphicsD3D12() final { wait_for_idle(); }

    GraphicsD3D12::GraphicsD3D12(const GraphicsD3D12&) = delete;
    GraphicsD3D12& operator=(const GraphicsD3D12&) = delete;
    GraphicsD3D12::GraphicsD3D12(GraphicsD3D12&&) = delete;
    GraphicsD3D12& operator=(GraphicsD3D12&&) = delete;

    bool create_swap_chain(void* window, void* /*application*/) final
    {
        Expects(_device);
        Expects(_render_queue);
        Expects(_factory);

        if (window == nullptr) {
            return false;
        }

        auto hwnd = static_cast<HWND>(window);
        constexpr DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {
            0,                           // Width
            0,                           // Height
            DXGI_FORMAT_B8G8R8A8_UNORM,  // Format
            FALSE,                       // Stereo
            {
                1,                            // Count
                0,                            // Quality
            },                                // SampleDesc
            DXGI_USAGE_RENDER_TARGET_OUTPUT,  // BufferUsage
            kFramesInFlight,                  // BufferCount
            DXGI_SCALING_NONE,                // Scaling
            DXGI_SWAP_EFFECT_FLIP_DISCARD,    // SwapEffect
            DXGI_ALPHA_MODE_UNSPECIFIED,      // AlphaMode
            0,                                // Flags
        };
        CComPtr<IDXGISwapChain1> swapchain1 = nullptr;
        HRESULT hr = _factory->CreateSwapChainForHwnd(_render_queue, hwnd, &swap_chain_desc,
                                                      nullptr, nullptr, &swapchain1);
        assert(SUCCEEDED(hr) && "Could not create swap chain");
        hr = swapchain1->QueryInterface(IID_PPV_ARGS(&_swap_chain));
        assert(SUCCEEDED(hr) && "Could not get swapchain3");
        hr = _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        assert(SUCCEEDED(hr) && "Could not disable alt-enter");

        set_name(_swap_chain, "DXGI Swap Chain");
        _swap_chain->GetDesc1(&_swap_chain_desc);

        return true;
    }

    bool resize(int /*width*/, int /*height*/) final
    {
        Expects(_swap_chain);
        if (_swap_chain == nullptr) {
            return false;
        }
        wait_for_idle();

        for (auto& buffer : _back_buffers) {
            buffer.Release();
        }

        HRESULT hr = _swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        assert(SUCCEEDED(hr) && "Could not resize buffers");

        _swap_chain->GetDesc1(&_swap_chain_desc);
        for (UINT ii = 0; ii < _swap_chain_desc.BufferCount; ++ii) {
            auto& buffer = gsl::at(_back_buffers, ii);

            hr = _swap_chain->GetBuffer(ii, IID_PPV_ARGS(&buffer));
            assert(SUCCEEDED(hr) && "Could not get back buffer");
            set_name(buffer, "Back Buffer %d", ii);

            auto const back_buffer_slot = _rtv_heap.cpu_slot(ii);
            constexpr D3D12_RENDER_TARGET_VIEW_DESC view_desc = {
                DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, D3D12_RTV_DIMENSION_TEXTURE2D,
            };
            _device->CreateRenderTargetView(buffer, &view_desc, back_buffer_slot);
        }
        return true;
    }

    bool present() final
    {
        if (_swap_chain == nullptr) {
            return false;
        }

        constexpr DXGI_PRESENT_PARAMETERS params = {
            0, nullptr, nullptr, nullptr,
        };
        HRESULT const hr = _swap_chain->Present1(0, 0, &params);
        return SUCCEEDED(hr);
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
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, FALSE);
            _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
        }

        CComPtr<ID3D12Debug1> d3d12_debug;
        D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12_debug));
        if (d3d12_debug) {
            d3d12_debug->EnableDebugLayer();
            d3d12_debug->SetEnableGPUBasedValidation(TRUE);
            d3d12_debug->SetEnableSynchronizedCommandQueueValidation(TRUE);
        }
#endif  // _DEBUG
    }

    void create_factory()
    {
        constexpr UINT factory_flags =
#if defined(_DEBUG)
            DXGI_CREATE_FACTORY_DEBUG |
#endif  // _DEBUG
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
            auto& adapter = gsl::at(_adapters, ii);
            CComPtr<IDXGIAdapter1> adapter1 = nullptr;
            if (_factory->EnumAdapters1(static_cast<UINT>(ii), &adapter1) == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            CComPtr<IDXGIAdapter3> adapter3 = nullptr;
            hr = adapter1->QueryInterface(&adapter3);
            assert(SUCCEEDED(hr) && "Could not query for adapter3");
            adapter3->GetDesc2(&adapter.desc);
            if ((adapter.desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
                continue;
            }

            adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                                           &adapter.memory_info);
            set_name(adapter3, "%S", adapter.desc.Description);

            // Set properties
            adapter.adapter = adapter3;
        }
    }

    void create_device()
    {
        Expects(_factory);
        for (auto& adapter : _adapters) {
            if (adapter.adapter == nullptr) {
                break;
            }
            HRESULT hr =
                D3D12CreateDevice(adapter.adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
            if (SUCCEEDED(hr)) {
                constexpr D3D_FEATURE_LEVEL levels[] = {
                    D3D_FEATURE_LEVEL_9_1,  D3D_FEATURE_LEVEL_9_2,  D3D_FEATURE_LEVEL_9_3,
                    D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0,
                    D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_12_1};
                D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {
                    _countof(levels), levels,  // NOLINT
                };
                hr = _device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels,
                                                  sizeof(featureLevels));
                assert(SUCCEEDED(hr));
                _feature_level = featureLevels.MaxSupportedFeatureLevel;
                _current_adapter = &adapter;
                if (developer_mode_enabled()) {
                    _device->SetStablePowerState(TRUE);
                }
                set_name(_device, "D3D12 device");
                break;
            }
        }
    }

    void create_queue()
    {
        Expects(_device);
        constexpr D3D12_COMMAND_QUEUE_DESC queue_desc = {
            D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
            D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
        HRESULT hr = _device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_render_queue));
        assert(SUCCEEDED(hr) && "Could not create queue");
        set_name(_render_queue, "Render Queue");

        hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_render_fence));
        assert(SUCCEEDED(hr) && "Could not create render fence");
        set_name(_render_fence, "Render Fence");
    }

    void create_descriptor_heaps()
    {
        constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128,
                                                          D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
        _rtv_heap = CreateDescriptorHeap(_device, heap_desc, "RTV Heap");
    }

    void wait_for_idle()
    {
        Expects(_render_queue);
        _last_fence_completion++;
        _render_queue->Signal(_render_fence, _last_fence_completion);
        while (_render_fence->GetCompletedValue() < _last_fence_completion) {
            continue;  // NOLINT
        }
    }

    //
    // Constants
    //
    static constexpr size_t kMaxAdapters = 4;
    static constexpr UINT kFramesInFlight = 3;

    //
    // Types
    //
    struct Adapter
    {
        CComPtr<IDXGIAdapter3> adapter = {};
        DXGI_ADAPTER_DESC2 desc = {};
        DXGI_QUERY_VIDEO_MEMORY_INFO memory_info = {};
    };

    //
    // Data members
    //
    CComPtr<IDXGIFactory4> _factory;
    std::array<Adapter, kMaxAdapters> _adapters;
    Adapter* _current_adapter = nullptr;

    CComPtr<ID3D12Device> _device;
    D3D_FEATURE_LEVEL _feature_level = D3D_FEATURE_LEVEL_9_1;
    CComPtr<ID3D12CommandQueue> _render_queue;
    CComPtr<ID3D12Fence> _render_fence;
    uint64_t _last_fence_completion = 0;

    DescriptorHeap _rtv_heap;

    CComPtr<IDXGISwapChain3> _swap_chain;
    DXGI_SWAP_CHAIN_DESC1 _swap_chain_desc = {};
    std::array<CComPtr<ID3D12Resource>, kFramesInFlight> _back_buffers;

#if defined(_DEBUG)
    CComPtr<IDXGIDebug1> _dxgi_debug;
    CComPtr<IDXGIInfoQueue> _dxgi_info_queue;
    CComPtr<ID3D12InfoQueue> d3d12InfoQueue;
#endif  // _DEBUG
};

ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

}  // namespace ak
