#ifndef _AK_COMMANDBUFFER_D3D12_H_
#define _AK_COMMANDBUFFER_D3D12_H_
#include "graphics/graphics.h"

#include <cassert>
#include <atlbase.h>
#include <d3d12.h>

namespace ak {

class CommandBufferD3D12 : public CommandBuffer
{
   public:
    void reset() final;
    bool begin_render_pass() final;

   private:
    friend class GraphicsD3D12;

    GraphicsD3D12* _graphics = nullptr;

    CComPtr<ID3D12CommandAllocator> _allocator;
    CComPtr<ID3D12GraphicsCommandList> _list;
    uint64_t _completion = 0;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_D3D12_H_
