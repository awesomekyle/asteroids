#include "graphics-d3d12.h"
#include "graphics/graphics.h"

#include <iostream>
#include <d3dcompiler.h>
#include <gsl/gsl>

#define UNUSED(v) ((void)(v))

namespace {

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
                               static_cast<UINT>(strnlen(buffer, sizeof(buffer))),  // NOLINT
                               buffer);                                             // NOLINT
    }
}

template<typename I>
CComPtr<I> create_dxgi_debug_interface()
{
    auto dxgi_debugModule = LoadLibraryA("dxgidebug.dll");
    if (dxgi_debugModule == nullptr) {
        return nullptr;
    }

    auto const pDXGIGetDebugInterface =
        GetProcAddressSafe<decltype(&DXGIGetDebugInterface)>(dxgi_debugModule,
                                                             "DXGIGetDebugInterface");
    if (pDXGIGetDebugInterface == nullptr) {
        return nullptr;
    }
    CComPtr<I> dxgi_interface;
    HRESULT const hr = pDXGIGetDebugInterface(IID_PPV_ARGS(&dxgi_interface));
    assert(SUCCEEDED(hr) && "Could not create DXGIDebug interface");
    UNUSED(hr);
    return dxgi_interface;
}

ak::DescriptorHeap CreateDescriptorHeap(ID3D12Device* const device,
                                        D3D12_DESCRIPTOR_HEAP_DESC const& desc,
                                        char const* const name = nullptr)
{
    Expects(device);
    ak::DescriptorHeap heap = {};
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

CComPtr<ID3DBlob> compile_hlsl_shader(char const* const sourceCode, char const* const target,
                                      char const* const entrypoint)
{
    Expects(sourceCode);
    Expects(target);
    Expects(entrypoint);
    auto const compiler_module = LoadLibraryA("d3dcompiler_47.dll");
    Ensures(compiler_module && "Need a D3D compiler");
    decltype(D3DCompile)* const pD3DCompile =
        reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(compiler_module, "D3DCompile"));
    Ensures(pD3DCompile && "Can't find D3DCompile method");

    UINT compiler_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS |
                          D3DCOMPILE_WARNINGS_ARE_ERRORS |
                          D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#if _DEBUG
    compiler_flags |= D3DCOMPILE_DEBUG;
#endif
    size_t const source_code_length = strnlen(sourceCode, 1024 * 1024);
    CComPtr<ID3DBlob> shader_blog = nullptr;
    CComPtr<ID3DBlob> error_blob = nullptr;
    HRESULT const hr =
        pD3DCompile(sourceCode, source_code_length, nullptr, nullptr, nullptr, entrypoint, target,
                    compiler_flags, 0, &shader_blog, &error_blob);
    if (error_blob) {
        std::cerr << "Eror compiling shader: "
                  << static_cast<char const*>(error_blob->GetBufferPointer()) << std::endl;
    }
    return shader_blog;
}

}  // anonymous namespace

namespace ak {

GraphicsD3D12::GraphicsD3D12()
    : _device(nullptr)
    , _render_queue(nullptr)
{
    create_debug_interfaces();
    create_factory();
    find_adapters();
    create_device();
    create_queue();
    create_descriptor_heaps();
    create_command_buffers();
}
GraphicsD3D12::~GraphicsD3D12()
{
    wait_for_idle();
}

Graphics::API GraphicsD3D12::api_type() const
{
    return kD3D12;
}

bool GraphicsD3D12::create_swap_chain(void* window, void* /*application*/)
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
    HRESULT hr = _factory->CreateSwapChainForHwnd(_render_queue, hwnd, &swap_chain_desc, nullptr,
                                                  nullptr, &swapchain1);
    assert(SUCCEEDED(hr) && "Could not create swap chain");
    hr = swapchain1->QueryInterface(IID_PPV_ARGS(&_swap_chain));
    assert(SUCCEEDED(hr) && "Could not get swapchain3");
    hr = _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(hr) && "Could not disable alt-enter");

    set_name(_swap_chain, "DXGI Swap Chain");
    _swap_chain->GetDesc1(&_swap_chain_desc);

    // trigger a resize to get the back buffers
    resize(0, 0);

    return true;
}

bool GraphicsD3D12::resize(int /*width*/, int /*height*/)
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

bool GraphicsD3D12::present()
{
    if (_swap_chain == nullptr) {
        return false;
    }

    constexpr DXGI_PRESENT_PARAMETERS params = {
        0, nullptr, nullptr, nullptr,
    };
    HRESULT const hr = _swap_chain->Present1(0, 0, &params);
    _current_back_buffer = {0, nullptr};
    return SUCCEEDED(hr);
}

CommandBuffer* GraphicsD3D12::command_buffer()
{
    Expects(_device);
    uint_fast32_t const curr_index = _current_command_buffer.fetch_add(1) % kMaxCommandBuffers;
    auto& buffer = gsl::at(_command_lists, curr_index);
    uint64_t const last_completed_value = _render_fence->GetCompletedValue();
    if (last_completed_value < buffer._completion) {
        /// @TODO: This case needs to be handled
        // assert(last_completed_value >= buffer._completion && "Oldest command buffer not yet
        // completed. Continue through the queue?");
        return nullptr;
    }
    buffer._completion = UINT64_MAX;
    HRESULT hr = buffer._allocator->Reset();
    assert(SUCCEEDED(hr) && "Could not reset allocator");
    hr = buffer._list->Reset(buffer._allocator, nullptr);
    assert(SUCCEEDED(hr) && "Could not reset command list");
    return &buffer;
}
int GraphicsD3D12::num_available_command_buffers()
{
    Expects(_device);
    int free_buffers = 0;
    for (auto const& command_buffer : _command_lists) {
        if (_render_fence->GetCompletedValue() >= command_buffer._completion) {
            free_buffers++;
        }
    }
    return free_buffers;
}
bool GraphicsD3D12::execute(CommandBuffer* command_buffer)
{
    Expects(_device);
    auto* const d3d12_buffer = static_cast<CommandBufferD3D12*>(command_buffer);  // NOLINT
    HRESULT const hr = d3d12_buffer->_list->Close();
    assert(SUCCEEDED(hr) && "Could not close command list");

    _render_queue->ExecuteCommandLists(1, CommandListCast(&d3d12_buffer->_list.p));
    d3d12_buffer->_completion = _last_fence_completion++;
    _render_queue->Signal(_render_fence, d3d12_buffer->_completion);
    return SUCCEEDED(hr);
}

std::unique_ptr<RenderState> GraphicsD3D12::create_render_state(RenderStateDesc const& desc)
{
    auto state = std::make_unique<RenderStateD3D12>();

    // Compile shaders
    CComPtr<ID3DBlob> vs_bytecode =
        compile_hlsl_shader(desc.vertex_shader.source, "vs_5_1", desc.vertex_shader.entrypoint);
    CComPtr<ID3DBlob> ps_bytecode =
        compile_hlsl_shader(desc.pixel_shader.source, "ps_5_1", desc.pixel_shader.entrypoint);

    // Root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(D3D12_DEFAULT);

    CComPtr<ID3DBlob> signature;
    CComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &signature, &error);
    assert(SUCCEEDED(hr));
    hr = _device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                      IID_PPV_ARGS(&state->_root_signature));
    assert(SUCCEEDED(hr));

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC const pso_desc = {
        state->_root_signature,                // pRootSignature
        CD3DX12_SHADER_BYTECODE(vs_bytecode),  // VS
        CD3DX12_SHADER_BYTECODE(ps_bytecode),  // PS
        {nullptr, 0},                          // DS
        {nullptr, 0},                          // HS
        {nullptr, 0},                          // GS
        {
            // StreamOutput
            nullptr,  // pSODeclaration
            0,        // NumEntries
            nullptr,  // pBufferStrides
            0,        // NumStrides
            0,        // RasterizedStream
        },
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),            // BlendState
        UINT_MAX,                                     // SampleMask
        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),       // RasterizerState
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),    // DepthStencilState
        {nullptr, 0},                                 // InputLayout
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,  // IBStripCutValue
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,       // PrimitiveTopologyType
        1,                                            // NumRenderTargets
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB},            // RTVFormats TODO: Parameterize
        DXGI_FORMAT_UNKNOWN,                          // DSVFormat
        {1, 0},                                       // SampleDesc
        0,                                            // NodeMask
        {nullptr, 0},                                 // CachedPSO
        D3D12_PIPELINE_STATE_FLAG_NONE,               // Flags
    };

    hr = _device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&state->_state));
    if (FAILED(hr)) {
        return nullptr;
    }

    return state;
}

void GraphicsD3D12::create_debug_interfaces()
{
#if defined(_DEBUG)
    _dxgi_debug = create_dxgi_debug_interface<IDXGIDebug1>();
    _dxgi_info_queue = create_dxgi_debug_interface<IDXGIInfoQueue>();
    if (_dxgi_debug) {
        _dxgi_debug->EnableLeakTrackingForThread();
    }
    if (_dxgi_info_queue) {
        _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR,
                                             TRUE);
        _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                             DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                             DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE);
        _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO,
                                             FALSE);
        _dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                             DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, FALSE);
    }

    CComPtr<ID3D12Debug1> d3d12_debug;
    D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12_debug));
    if (d3d12_debug) {
        d3d12_debug->EnableDebugLayer();
        // d3d12_debug->SetEnableGPUBasedValidation(TRUE);
        // d3d12_debug->SetEnableSynchronizedCommandQueueValidation(TRUE);
    }
#endif  // _DEBUG
}

void GraphicsD3D12::create_factory()
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

void GraphicsD3D12::find_adapters()
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

        adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &adapter.memory_info);
        set_name(adapter3, "%S", adapter.desc.Description);

        // Set properties
        adapter.adapter = adapter3;
    }
}

void GraphicsD3D12::create_device()
{
    Expects(_factory);
    for (auto& adapter : _adapters) {
        if (adapter.adapter == nullptr) {
            break;
        }
        HRESULT hr =
            D3D12CreateDevice(adapter.adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
        if (SUCCEEDED(hr)) {
            constexpr D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_9_1,  D3D_FEATURE_LEVEL_9_2,
                                                    D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_10_0,
                                                    D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0,
                                                    D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_12_0,
                                                    D3D_FEATURE_LEVEL_12_1};
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

void GraphicsD3D12::create_queue()
{
    Expects(_device);
    constexpr D3D12_COMMAND_QUEUE_DESC queue_desc =
        {D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
         D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    HRESULT hr = _device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_render_queue));
    assert(SUCCEEDED(hr) && "Could not create queue");
    set_name(_render_queue, "Render Queue");

    hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_render_fence));
    assert(SUCCEEDED(hr) && "Could not create render fence");
    set_name(_render_fence, "Render Fence");
}

void GraphicsD3D12::create_descriptor_heaps()
{
    Expects(_device);
    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128,
                                                      D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
    _rtv_heap = CreateDescriptorHeap(_device, heap_desc, "RTV Heap");
}

void GraphicsD3D12::create_command_buffers()
{
    Expects(_device);
    int index = 0;
    for (auto& command_buffer : _command_lists) {
        HRESULT hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(&command_buffer._allocator));
        assert(SUCCEEDED(hr) && "Could not create allocator");
        hr =
            _device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, command_buffer._allocator,
                                       nullptr, IID_PPV_ARGS(&command_buffer._list));
        assert(SUCCEEDED(hr) && "Could not create allocator");
        hr = command_buffer._list->Close();
        assert(SUCCEEDED(hr) && "Could not close empty command list");

        set_name(command_buffer._allocator, "Command Allocator %zu", index);
        set_name(command_buffer._list, "Command list %zu", index);

        command_buffer._graphics = this;
        index++;
    }
}

void GraphicsD3D12::wait_for_idle()
{
    Expects(_render_queue);
    _last_fence_completion++;
    _render_queue->Signal(_render_fence, _last_fence_completion);
    while (_render_fence->GetCompletedValue() < _last_fence_completion) {
        continue;  // NOLINT
    }
}

std::pair<uint32_t, ID3D12Resource*> const& GraphicsD3D12::current_back_buffer()
{
    if (_current_back_buffer.second == nullptr) {
        auto const frame_index = _swap_chain->GetCurrentBackBufferIndex();
        ID3D12Resource* const back_buffer = gsl::at(_back_buffers, frame_index);
        _current_back_buffer = {frame_index, back_buffer};
    }
    return _current_back_buffer;
}

std::pair<uint32_t, uint32_t> GraphicsD3D12::get_dimensions()
{
    return std::make_pair(_swap_chain_desc.Width, _swap_chain_desc.Height);
}

ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

}  // namespace ak
