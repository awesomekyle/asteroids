#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct Gfx Gfx; ///< Object representing a graphics device (ID3D11Device, vkDevice, MTLDevice)

/// @brief Creates a new graphics device
Gfx* gfxCreate(void);
/// @brief Terminates and destroys a graphics device
void gfxDestroy(Gfx* G);

/// @brief Creates a swap chain for the specified window.
/// @details Windows D3D: IDXGISwapChain
///          macOS Metal: CAMetalLayer
///          Windows Vulkan: vkSwapChainKHR
/// @param[in] window Native window handle (Windows: HWND, macOS: NSWindow*)
bool gfxCreateSwapChain(Gfx* G, void* window);

/// @brief Called when the window is resized.
/// @details Pass in window size in pixels, not points
bool gfxResize(Gfx* G, int width, int height);
