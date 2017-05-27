#ifndef _AK_GRAPHICS_H_
#define _AK_GRAPHICS_H_
#include <memory>

namespace ak {

class CommandBuffer
{
   public:
};

class Graphics
{
   public:
    virtual ~Graphics();

    ///
    /// Constants
    ///
    static constexpr size_t kMaxCommandBuffers = 128;

    /// @brief Creates a swap chain for the specified window.
    /// @details Windows D3D: IDXGISwapChain
    ///          macOS Metal: CAMetalLayer
    ///          Windows Vulkan: vkSwapChainKHR
    /// @param[in] window Native window handle (Windows: HWND, macOS: NSWindow*)
    /// @param[in] application Native application handle (Windows: HINSTANCE, macOS: NSApplication*)
    virtual bool create_swap_chain(void* window, void* application) = 0;

    /// @brief Call when the window is resized.
    /// @details Pass in window size in pixels, not points
    virtual bool resize(int width, int height) = 0;

    /// @brief Presents the back buffer to the screen
    virtual bool present() = 0;

    /// @brief Returns an open, ready to use command buffer
    /// @return NULL if no command buffers are available
    virtual CommandBuffer* command_buffer() = 0;
};

using ScopedGraphics = std::unique_ptr<Graphics>;

ScopedGraphics create_graphics();

}  // namespace ak

#endif  //_AK_GRAPHICS_H_