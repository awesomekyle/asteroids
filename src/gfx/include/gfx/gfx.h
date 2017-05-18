#pragma once
#include <stdint.h>
#include <stdbool.h>

/// @TODO:
///     Vulkan object naming (via extension)
///     Vulkan invalid render target handling

typedef enum GfxApi {
    kGfxApiDefault = 0,
    kGfxApiD3D12,
    kGfxApiMetal,
    kGfxApiVulkan,

    kGfxApiUnknown = -1,
} GfxApi;


/// @brief 3D graphics formats
typedef enum GfxPixelFormat {
    kUnknown = 0,
    // r8
    kR8Uint,
    kR8Sint,
    kR8SNorm,
    kR8UNorm,
    // rg8
    kRG8Uint,
    kRG8Sint,
    kRG8SNorm,
    kRG8UNorm,
    // r8g8b8
    kRGBX8Uint,
    kRGBX8Sint,
    kRGBX8SNorm,
    kRGBX8UNorm,
    kRGBX8UNorm_sRGB,
    // r8g8b8a8
    kRGBA8Uint,
    kRGBA8Sint,
    kRGBA8SNorm,
    kRGBA8UNorm,
    kRGBA8UNorm_sRGB,
    // b8g8r8a8
    kBGRA8UNorm,
    kBGRA8UNorm_sRGB,
    // r16
    kR16Uint,
    kR16Sint,
    kR16Float,
    kR16SNorm,
    kR16UNorm,
    // r16g16
    kRG16Uint,
    kRG16Sint,
    kRG16Float,
    kRG16SNorm,
    kRG16UNorm,
    // r16g16b16a16
    kRGBA16Uint,
    kRGBA16Sint,
    kRGBA16Float,
    kRGBA16SNorm,
    kRGBA16UNorm,
    // r32
    kR32Uint,
    kR32Sint,
    kR32Float,
    // r32g32
    kRG32Uint,
    kRG32Sint,
    kRG32Float,
    // r32g32b32x32
    kRGBX32Uint,
    kRGBX32Sint,
    kRGBX32Float,
    // r32g32b32a32
    kRGBA32Uint,
    kRGBA32Sint,
    kRGBA32Float,
    // rbg10a2
    kRGB10A2Uint,
    kRGB10A2UNorm,
    // r11g11b10
    kRG11B10Float,
    // rgb9e5
    kRGB9E5Float,
    // bgr5a1
    kBGR5A1Unorm,
    // b5g6r5
    kB5G6R5Unorm,
    // b4g4r4a4
    kBGRA4Unorm,
    // bc1-7
    kBC1Unorm,
    kBC1Unorm_SRGB,
    kBC2Unorm,
    kBC2Unorm_SRGB,
    kBC3Unorm,
    kBC3Unorm_SRGB,
    kBC4Unorm,
    kBC4Snorm,
    kBC5Unorm,
    kBC5Snorm,
    kBC6HUfloat,
    kBC6HSfloat,
    kBC7Unorm,
    kBC7Unorm_SRGB,
    // d24s8
    kD24UnormS8Uint,
    // d32
    kD32Float,
    // d32s8
    kD32FloatS8Uint,

    kNumFormats
} GfxPixelFormat;

typedef struct GfxInputLayout {
    char const* name;
    uint32_t slot;
    GfxPixelFormat format;
} GfxInputLayout;

static GfxInputLayout const kEndLayout = {
    NULL, 0, kUnknown,
};

typedef enum GfxCullMode {
    kBack,
    kFront,
    kNone
} GfxCullMode;

typedef struct GfxShaderDesc {
    char const* source;
    char const* entrypoint;
} GfxShaderDesc;

typedef struct GfxRenderStateDesc {
    GfxShaderDesc vertexShader;
    GfxShaderDesc pixelShader;
    GfxInputLayout const* layout;
    GfxPixelFormat depthFormat;
    GfxCullMode culling;
    bool depthWrite;
    char const* name;
} GfxRenderStateDesc;


/// Object representing a graphics device (ID3D11Device, vkDevice, MTLDevice)
typedef struct Gfx Gfx;
/// Object representing a graphics command buffer/command list
typedef struct GfxCmdBuffer GfxCmdBuffer;

/// Opaque object handle
static intptr_t const kGfxInvalidHandle = -1;
typedef intptr_t GfxRenderTarget;

//------------------------- Primary Interface ----------------------------------

/// @brief Creates a new graphics device
Gfx* gfxCreate(GfxApi api);
/// @brief Terminates and destroys a graphics device
void gfxDestroy(Gfx* G);

/// @brief Creates a swap chain for the specified window.
/// @details Windows D3D: IDXGISwapChain
///          macOS Metal: CAMetalLayer
///          Windows Vulkan: vkSwapChainKHR
/// @param[in] window Native window handle (Windows: HWND, macOS: NSWindow*)
/// @param[in] application Native application handle (Windows: HINSTANCE, macOS: NSApplication*)
bool gfxCreateSwapChain(Gfx* G, void* window, void* application);

/// @brief Called when the window is resized.
/// @details Pass in window size in pixels, not points
bool gfxResize(Gfx* G, int width, int height);

/// @brief Returns the next back buffer
GfxRenderTarget gfxGetBackBuffer(Gfx* G);

/// @brief Presents the back buffer to the screen
bool gfxPresent(Gfx* G);

/// @brief Returns an open, ready to use command buffer
/// @return NULL if no command buffers are available
GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G);

/// @brief Returns a rough guess of currently available command buffers
/// @note This method is not atomic, due to threading, this number may not be
///     exact.
int gfxNumAvailableCommandBuffers(Gfx* G);

/// @brief Resets a command buffer to restore it to the Gfx device without
///     execution
/// @note After this call, the buffer should no longer be accessed
void gfxResetCommandBuffer(GfxCmdBuffer* B);

/// @brief Executes a command buffer on the GPU
bool gfxExecuteCommandBuffer(GfxCmdBuffer* B);

//------------------ Command Buffer Interface ----------------------------------

typedef enum GfxRenderPassAction {
    kGfxRenderPassActionClear,
} GfxRenderPassAction;

/// @brief Begins a "render pass", a collection of API calls that all occur on
///     the same render target set.
void gfxCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4]);
void gfxCmdEndRenderPass(GfxCmdBuffer* B);
