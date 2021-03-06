#ifndef _AK_GRAPHICS_D3D12_H_
#define _AK_GRAPHICS_D3D12_H_
#include "graphics/graphics.h"

#include <array>
#include <atomic>
#include <gsl/gsl_assert>

#include <d3d12.h>
#include <dxgi1_6.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <atlbase.h>

#pragma warning(push)
#pragma warning(disable : 4324)  // structure padded
#include <d3dx12.h>
#pragma warning(pop)

#include "command-buffer-d3d12.h"

namespace ak {

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
        return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(
            CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_start, slot, handleSize));
    }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GpuSlot(int slot)
    {
        return static_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(
            CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_start, slot, handleSize));
    }
    explicit operator ID3D12DescriptorHeap*() { return heap; }
};

class RenderStateD3D12 : public RenderState
{
   public:
    CComPtr<ID3D12RootSignature> _root_signature;
    CComPtr<ID3D12PipelineState> _state;
};

class BufferD3D12 : public Buffer
{
   public:
    CComPtr<ID3D12Resource> _buffer;
};

class GraphicsD3D12 : public Graphics
{
   public:
    GraphicsD3D12();
    ~GraphicsD3D12() final;

    API api_type() const final;
    bool create_swap_chain(void* window, void*) final;
    bool resize(int, int) final;
    bool present() final;
    CommandBuffer* command_buffer() final;
    int num_available_command_buffers() final;
    bool execute(CommandBuffer* command_buffer) final;
    void wait_for_idle() final;
    void* get_upload_data(size_t const size, size_t const alignment) final;

    std::unique_ptr<RenderState> create_render_state(RenderStateDesc const& desc) final;
    std::unique_ptr<Buffer> create_vertex_buffer(uint32_t size, void const* data) final;
    std::unique_ptr<Buffer> create_index_buffer(uint32_t size, void const* data) final;

   private:
    friend class CommandBufferD3D12;

    GraphicsD3D12(const GraphicsD3D12&) = delete;
    GraphicsD3D12& operator=(const GraphicsD3D12&) = delete;
    GraphicsD3D12(GraphicsD3D12&&) = delete;
    GraphicsD3D12& operator=(GraphicsD3D12&&) = delete;

    void create_debug_interfaces();
    void create_factory();
    void find_adapters();
    void create_device();
    void create_queue();
    void create_descriptor_heaps();
    void create_command_buffers();

    std::pair<uint32_t, ID3D12Resource*> const& current_back_buffer();
    std::pair<uint32_t, uint32_t> get_dimensions();

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
    std::pair<uint32_t, ID3D12Resource*> _current_back_buffer = {};

    std::array<CommandBufferD3D12, kMaxCommandBuffers> _command_lists;
    std::atomic<uint32_t> _current_command_buffer = {};

#if defined(_DEBUG)
    CComPtr<IDXGIDebug1> _dxgi_debug;
    CComPtr<IDXGIInfoQueue> _dxgi_info_queue;
    CComPtr<ID3D12InfoQueue> d3d12InfoQueue;
#endif
    // _DEBUG
};

/// @brief Creates a D3D12 Graphics device
ScopedGraphics create_graphics_d3d12();

}  // namespace ak

#endif  // _AK_GRAPHICS_D3D12_H_
