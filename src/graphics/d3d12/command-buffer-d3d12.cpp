#include "command-buffer-d3d12.h"
#include "graphics-d3d12.h"

#include <gsl/gsl>

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

    auto const& current_back_buffer = _graphics->current_back_buffer();
    auto const rt_descriptor = _graphics->_rtv_heap.cpu_slot(current_back_buffer.first);

    // transition from present to RT
    auto const barrier = CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer.second,
                                                              D3D12_RESOURCE_STATE_PRESENT,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET);

    _list->ResourceBarrier(1, &barrier);

    constexpr float clear_color[] = {
        0.0f, 0.75f, 1.0f, 1.0f,
    };
    _list->ClearRenderTargetView(rt_descriptor, gsl::make_span(clear_color).data(), 0, nullptr);
    _list->OMSetRenderTargets(1, &rt_descriptor, 0, nullptr);

    // Set up global options
    auto const dimensions = _graphics->get_dimensions();
    D3D12_RECT const scissor = {
        0, 0, static_cast<LONG>(dimensions.first), static_cast<LONG>(dimensions.second),
    };
    _list->RSSetScissorRects(1, &scissor);
    D3D12_VIEWPORT const viewport = {
        0, 0, static_cast<float>(dimensions.first), static_cast<float>(dimensions.second), 0, 1,
    };
    _list->RSSetViewports(1, &viewport);
    _list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return true;
}

void CommandBufferD3D12::set_render_state(RenderState* const state)
{
    auto* const d3d12_state = static_cast<RenderStateD3D12*>(state);
    _list->SetGraphicsRootSignature(d3d12_state->_root_signature);
    _list->SetPipelineState(d3d12_state->_state);
}

void CommandBufferD3D12::set_vertex_buffer(Buffer* const /*buffer*/)
{
    // UNIMPLEMENTED
}

void CommandBufferD3D12::set_index_buffer(Buffer* const /*buffer*/)
{
    // UNIMPLEMENTED
}

void CommandBufferD3D12::draw_indexed(uint32_t const /*index_count*/)
{
    // UNIMPLEMENTED
}

void CommandBufferD3D12::draw(uint32_t const vertex_count)
{
    _list->DrawInstanced(vertex_count, 1, 0, 0);
}

void CommandBufferD3D12::end_render_pass()
{
    auto const& current_back_buffer = _graphics->current_back_buffer();
    // transition back to present
    auto const barrier = CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer.second,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                              D3D12_RESOURCE_STATE_PRESENT);

    _list->ResourceBarrier(1, &barrier);
}

}  // namespace ak
