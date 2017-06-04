#pragma once
#include "graphics/graphics.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4201)  // nameless struct/union
#endif
#include <mathfu/hlsl_mappings.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct GLFWWindow;

class Application
{
   public:
    Application(void* native_window, void* native_instance);
    ~Application();

    void on_resize(int width, int height);
    void on_frame(float delta_time);

   private:
    struct ConstantBuffer
    {
        mathfu::float4x4 projection;
        mathfu::float4x4 view;
        mathfu::float4x4 world;
    };

    struct Model
    {
        std::unique_ptr<ak::Buffer> vertex_buffer;
        std::unique_ptr<ak::Buffer> index_buffer;
        uint32_t index_count;
        uint32_t vertex_count;
    };

    void* const _window = nullptr;
    void* const _instance = nullptr;
    ak::ScopedGraphics const _graphics = nullptr;

    int _width = 0;
    int _height = 0;

    Model _cube_model = {};

    std::unique_ptr<ak::RenderState> _render_state;

    ConstantBuffer _constant_buffer = {};
};
