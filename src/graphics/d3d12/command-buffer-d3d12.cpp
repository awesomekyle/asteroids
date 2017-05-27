#include "command-buffer-d3d12.h"

namespace ak {

void CommandBufferD3D12::reset()
{
    auto const hr = _list->Close();
    assert(SUCCEEDED(hr) && "Could not close command list");
    _completion = 0;
}

bool CommandBufferD3D12::begin_render_pass()
{
    return false;
}

}  // namespace ak
