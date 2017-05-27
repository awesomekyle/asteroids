#include "command-buffer-d3d12.h"
#include "graphics-d3d12.h"

namespace ak {

void CommandBufferD3D12::reset()
{
    auto const hr = _list->Close();
    assert(SUCCEEDED(hr) && "Could not close command list");
    _completion = 0;
}

bool CommandBufferD3D12::begin_render_pass()
{
    if (_graphics->_swap_chain == nullptr) {
        return false;
    }

    auto const frame_index = _graphics->_swap_chain->GetCurrentBackBufferIndex();
    auto const rt_descriptor = _graphics->_rtv_heap.cpu_slot(frame_index);

    // transition from present to RT
    ID3D12Resource* const back_buffer = _graphics->_back_buffers[frame_index];

    auto const barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    _list->ResourceBarrier(1, &barrier);

    constexpr float clear_color[] = {
        0.0f, 0.75f, 1.0f, 1.0f,
    };
    _list->ClearRenderTargetView(rt_descriptor, clear_color, 0, nullptr);
    _list->OMSetRenderTargets(1, &rt_descriptor, false, nullptr);

    return true;
}

void CommandBufferD3D12::end_render_pass()
{
    auto const frame_index = _graphics->_swap_chain->GetCurrentBackBufferIndex();
    auto const rt_descriptor = _graphics->_rtv_heap.cpu_slot(frame_index);

    // transition back to present
    ID3D12Resource* const back_buffer = _graphics->_back_buffers[frame_index];

    auto const barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    _list->ResourceBarrier(1, &barrier);
}

}  // namespace ak
