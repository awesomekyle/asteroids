#ifndef _AK_COMMANDBUFFER_D3D12_H_
#define _AK_COMMANDBUFFER_D3D12_H_
#include "graphics/graphics.h"

#include <cassert>

namespace ak {

class CommandBufferD3D12 : public CommandBuffer
{
   public:
    CComPtr<ID3D12CommandAllocator> _allocator;
    CComPtr<ID3D12GraphicsCommandList> _list;
    uint64_t _completion = 0;

    void reset() final
    {
        auto const hr = _list->Close();
        assert(SUCCEEDED(hr) && "Could not close command list");
        _completion = 0;
    }
    bool begin_render_pass() final { return false; }
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_D3D12_H_
