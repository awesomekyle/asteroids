#include "config.h"
#include "gfx/gfx.h"
#include <stdbool.h>
#include <stdlib.h>
#if HAVE_D3D12
#   include "d3d12/gfx-d3d12.h"
#endif
#if HAVE_VULKAN
#   include "vulkan/gfx-vulkan.h"
#endif
#if HAVE_METAL
#   include "metal/gfx-metal.h"
#endif
#include "gfx-internal.h"

struct Gfx {
    GfxTable const* table;
};
struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
};

static GfxApi _PlatformDefaultApi()
{
#if defined(_WIN32)
    return kGfxApiD3D12;
#elif defined(__APPLE__)
    return kGfxApiMetal;
#else
#error Must specify default API
    return kGfxApiUnknown;
#endif
}

Gfx* gfxCreate(GfxApi api)
{
    if (api == kGfxApiDefault) {
        api = _PlatformDefaultApi();
    }
    switch (api) {
#if HAVE_D3D12
        case kGfxApiD3D12:
            return gfxD3D12Create();
#endif
#if HAVE_VULKAN
        case kGfxApiVulkan:
            return gfxVulkanCreate();
#endif
#if HAVE_METAL
        case kGfxApiMetal:
            return gfxMetalCreate();
#endif
        default:
            break;
    }
    return NULL;
}

void gfxDestroy(Gfx* G)
{
    G->table->Destroy(G);
}


bool gfxCreateSwapChain(Gfx* G, void* window, void* application)
{
    return G->table->CreateSwapChain(G, window, application);
}

bool gfxResize(Gfx* G, int width, int height)
{
    return G->table->Resize(G, width, height);
}

GfxRenderTarget gfxGetBackBuffer(Gfx* G)
{
    return G->table->GetBackBuffer(G);
}

bool gfxPresent(Gfx* G)
{
    return G->table->Present(G);
}

GfxRenderState* gfxCreateRenderState(Gfx* G, GfxRenderStateDesc const* desc)
{
    return G->table->CreateRenderState(G, desc);
}

GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G)
{
    return G->table->GetCommandBuffer(G);
}

int gfxNumAvailableCommandBuffers(Gfx* G)
{
    return G->table->NumAvailableCommandBuffers(G);
}

void gfxResetCommandBuffer(GfxCmdBuffer* B)
{
    B->table->ResetCommandBuffer(B);
}

bool gfxExecuteCommandBuffer(GfxCmdBuffer* B)
{
    return B->table->ExecuteCommandBuffer(B);
}

void gfxCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4])
{
    B->table->CmdBeginRenderPass(B, renderTargetHandle, loadAction, clearColor);
}
void gfxCmdEndRenderPass(GfxCmdBuffer* B)
{
    B->table->CmdEndRenderPass(B);
}

size_t gfxFormatSize(GfxPixelFormat format)
{
    switch (format) {
        case kUnknown:
            return 0;
        // r8
        case kR8Uint:
            return 1;
        case kR8Sint:
            return 1;
        case kR8SNorm:
            return 1;
        case kR8UNorm:
            return 1;
        // rg8
        case kRG8Uint:
            return 2;
        case kRG8Sint:
            return 2;
        case kRG8SNorm:
            return 2;
        case kRG8UNorm:
            return 2;
        // r8g8b8
        case kRGBX8Uint:
            return 4;
        case kRGBX8Sint:
            return 4;
        case kRGBX8SNorm:
            return 4;
        case kRGBX8UNorm:
            return 4;
        case kRGBX8UNorm_sRGB:
            return 4;
        // r8g8b8a8
        case kRGBA8Uint:
            return 4;
        case kRGBA8Sint:
            return 4;
        case kRGBA8SNorm:
            return 4;
        case kRGBA8UNorm:
            return 4;
        case kRGBA8UNorm_sRGB:
            return 4;
        // b8g8r8a8
        case kBGRA8UNorm:
            return 4;
        case kBGRA8UNorm_sRGB:
            return 4;
        // r16
        case kR16Uint:
            return 2;
        case kR16Sint:
            return 2;
        case kR16Float:
            return 2;
        case kR16SNorm:
            return 2;
        case kR16UNorm:
            return 2;
        // r16g16
        case kRG16Uint:
            return 4;
        case kRG16Sint:
            return 4;
        case kRG16Float:
            return 4;
        case kRG16SNorm:
            return 4;
        case kRG16UNorm:
            return 4;
        // r16g16b16a16
        case kRGBA16Uint:
            return 8;
        case kRGBA16Sint:
            return 8;
        case kRGBA16Float:
            return 8;
        case kRGBA16SNorm:
            return 8;
        case kRGBA16UNorm:
            return 8;
        // r32
        case kR32Uint:
            return 4;
        case kR32Sint:
            return 4;
        case kR32Float:
            return 4;
        // r32g32
        case kRG32Uint:
            return 8;
        case kRG32Sint:
            return 8;
        case kRG32Float:
            return 8;
        // r32g32b32x32
        case kRGBX32Uint:
            return 12;
        case kRGBX32Sint:
            return 12;
        case kRGBX32Float:
            return 12;
        // r32g32b32a32
        case kRGBA32Uint:
            return 16;
        case kRGBA32Sint:
            return 16;
        case kRGBA32Float:
            return 16;
        // rbg10a2
        case kRGB10A2Uint:
            return 4;
        case kRGB10A2UNorm:
            return 4;
        // r11g11b10
        case kRG11B10Float:
            return 4;
        // rgb9e5
        case kRGB9E5Float:
            return 4;
        // bgr5a1
        case kBGR5A1Unorm:
            return 2;
        // b5g6r5
        case kB5G6R5Unorm:
            return 2;
        // b4g4r4a4
        case kBGRA4Unorm:
            return 2;
        // bc1-7
        case kBC1Unorm:
            return 8;
        case kBC1Unorm_SRGB:
            return 8;
        case kBC2Unorm:
            return 16;
        case kBC2Unorm_SRGB:
            return 16;
        case kBC3Unorm:
            return 16;
        case kBC3Unorm_SRGB:
            return 16;
        case kBC4Unorm:
            return 8;
        case kBC4Snorm:
            return 8;
        case kBC5Unorm:
            return 16;
        case kBC5Snorm:
            return 16;
        case kBC6HUfloat:
            return 16;
        case kBC6HSfloat:
            return 16;
        case kBC7Unorm:
            return 16;
        case kBC7Unorm_SRGB:
            return 16;
        // d24s8
        case kD24UnormS8Uint:
            return 4;
        // d32
        case kD32Float:
            return 4;
        // d32s8
        case kD32FloatS8Uint:
            return 8;
        default:
            break;
    }
    return 0;
}
