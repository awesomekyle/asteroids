#ifndef _AK_GRAPHICS_H_
#define _AK_GRAPHICS_H_
#include <memory>

namespace ak {

struct ShaderDesc
{
    void const* bytecode;  ///< Shader bytecode
    size_t size;
};

struct InputLayout
{
    char const* name;
    uint32_t slot;
    uint32_t num_floats;  // TODO(kw): Support more than just float
};
static InputLayout const kEndLayout = {
    nullptr, 0, 0,
};
struct RenderStateDesc
{
    ShaderDesc vertex_shader;
    ShaderDesc pixel_shader;
    InputLayout const* input_layout;
    char const* name;
};

class CommandBuffer;
class RenderState;
class Graphics;
class Buffer;

class CommandBuffer
{
   public:
    virtual ~CommandBuffer();

    /// @brief Resets a command buffer to restore it to the Gfx device without execution
    /// @note After this call, the buffer should no longer be accessed
    virtual void reset() = 0;

    /// @brief Begins a "render pass", a collection of API calls that all occur on the final
    /// framebuffer.
    virtual bool begin_render_pass() = 0;

    /// @brief Sets constant buffer in vertex shader slot 0
    /// @param[in] upload_data A pointer previously retrieved from `get_upload_buffer`
    virtual void set_vertex_constant_data(uint32_t slot, void const* upload_data, size_t size) = 0;

    /// @brief Sets constant buffer in pixel shader slot 0
    /// @param[in] upload_data A pointer previously retrieved from `get_upload_buffer`
    virtual void set_pixel_constant_data(void const* upload_data, size_t size) = 0;

    /// Resource setting
    virtual void set_render_state(RenderState* const state) = 0;
    virtual void set_vertex_buffer(Buffer* const buffer) = 0;
    virtual void set_index_buffer(Buffer* const buffer) = 0;

    /// @brief Makes a non-indexed draw call
    virtual void draw(uint32_t vertex_count) = 0;

    /// @brief Makes an indexed draw call
    virtual void draw_indexed(uint32_t index_count) = 0;

    /// @brief Ends a previously started render pass
    virtual void end_render_pass() = 0;
};

/// Represents a render pipeline state (PSO for modern APIs, collection of states
///     in D3D11)
class RenderState
{
   public:
    virtual ~RenderState();
};

class Buffer
{
   public:
    virtual ~Buffer();
};

/// @todo Enable renaming of all graphics objects
class Graphics
{
   public:
    virtual ~Graphics();

    ///
    /// Constants
    ///
    enum {
        kMaxCommandBuffers = 128,
    };
    enum API {
        kDefault = 0,
        kD3D12,
        kVulkan,
        kMetal,

        kUnknown = -1,
    };

    virtual API api_type() const = 0;

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

    /// @brief Returns a rough guess of currently available command buffers
    /// @note This method is not atomic, due to threading, this number may not be
    ///     exact.
    virtual int num_available_command_buffers() = 0;

    /// @brief Executes a command buffer on the GPU
    virtual bool execute(CommandBuffer* command_buffer) = 0;

    /// @brief Waits on the CPU until the GPU is idle
    virtual void wait_for_idle() = 0;

    /// @brief Allocates memory from the upload buffer to use as constant buffer data
    virtual void* get_upload_data(size_t const size, size_t const alignment = 256) = 0;
    template<typename T>
    T* get_upload_data()
    {
        return static_cast<T*>(get_upload_data(sizeof(T)));
    }

    virtual std::unique_ptr<RenderState> create_render_state(RenderStateDesc const& desc) = 0;
    virtual std::unique_ptr<Buffer> create_vertex_buffer(uint32_t size, void const* data) = 0;
    virtual std::unique_ptr<Buffer> create_index_buffer(uint32_t size, void const* data) = 0;
};

using ScopedGraphics = std::unique_ptr<Graphics>;

ScopedGraphics create_graphics(Graphics::API api = Graphics::kDefault);

}  // namespace ak

#endif  //_AK_GRAPHICS_H_
