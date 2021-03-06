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
    void on_keyup(int glfw_key);
    void on_mouse_move(float delta_x, float delta_y);
    void on_scroll(float scroll);

   private:
    void recalculate_camera();

    //
    // constants
    //

    static constexpr float kSimOrbitRadius = 450.0f;
    static constexpr float kSimDiscRadius = 120.0f;
#if defined(_DEBUG)
    static constexpr int kNumAsteroids = 500;
#else
    static constexpr int kNumAsteroids = 50000;
#endif

    //
    // types
    //
    struct PerFrameConstants
    {
        mathfu::float4x4 projection;
        mathfu::float4x4 view;
        mathfu::float4x4 viewproj;
    };
    struct PerModelConstants
    {
        mathfu::float4x4 world;
    };
    struct PSConstantBuffer
    {
        mathfu::float4 color;
    };

    struct Asteroid
    {
        // TODO(kw): Split into static and dynamic data for better caching
        mathfu::float4x4 world;
        mathfu::float3 spin_axis;
        float scale;
        float spin_velocity;
        float orbit_velocity;
    };

    struct Model
    {
        std::unique_ptr<ak::Buffer> vertex_buffer;
        std::unique_ptr<ak::Buffer> index_buffer;
        uint32_t index_count;
    };

    //
    // data members
    //

    void* const _window = nullptr;
    void* const _instance = nullptr;
    ak::ScopedGraphics const _graphics = nullptr;

    bool _simulate = true;

    int _width = 0;
    int _height = 0;

    Model _cube_model = {};

    std::unique_ptr<ak::RenderState> _render_state;

    PerFrameConstants _constant_buffer = {};

    Asteroid _asteroids[kNumAsteroids] = {};

    // camera
    mathfu::float2 _cursor_delta = {0, 0};
    float _wheel_delta = 0.0f;

    float _cam_radius = kSimDiscRadius + kSimOrbitRadius + 10.0f;
    float _cam_long_angle = 4.5f;
    float _cam_lat_angle = 1.45f;
};
