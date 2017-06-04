#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#include <iostream>
#include <cassert>
#include <codecvt>
#include <fstream>
#include <vector>
#if defined(_WIN32)
#include <Windows.h>
#include <Pathcch.h>
#endif

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "graphics/graphics.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4201)  // nameless struct/union
#endif
#include <mathfu/hlsl_mappings.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace {

constexpr int kInitialWidth = 1920;
constexpr int kInitialHeight = 1080;

////
// GLFW helpers
////
void* native_window(GLFWwindow* const window)
{
#if defined(_WIN32)
    return glfwGetWin32Window(window);
#elif defined(__APPLE__)
    return glfwGetCocoaWindow(window);
#else
#warning "Not passing native window to Gfx"
    return nullptr;
#endif
}

void* native_instance()
{
#if defined(_WIN32)
    return static_cast<void*>(GetModuleHandle(nullptr));
#elif defined(__APPLE__)
    return nullptr;  // TODO(kw): return NSApp
#else
#warning "Not passing native application"
    return nullptr;
#endif
}

float get_window_scale(GLFWwindow* window)
{
    int windowWidth = 0;
    int framebufferWidth = 0;
    glfwGetWindowSize(window, &windowWidth, nullptr);
    glfwGetFramebufferSize(window, &framebufferWidth, nullptr);
    return static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth);
}
/// @brief Sets a windows framebuffer size. This may be different than the size
///     in "points" on high-DPI displays
void set_framebuffer_size(GLFWwindow* window, int width, int height)
{
    float const scale = get_window_scale(window);
    glfwSetWindowSize(window, static_cast<int>(width / scale), static_cast<int>(height / scale));
}
ak::Graphics* get_window_graphics(GLFWwindow* window)
{
    return static_cast<ak::Graphics*>(glfwGetWindowUserPointer(window));
}

std::string get_executable_directory()
{
#if defined(_WIN32)
    HMODULE const module = GetModuleHandleW(nullptr);
    wchar_t exe_name[2048] = {};
    GetModuleFileNameW(module, exe_name, sizeof(exe_name));
    HRESULT const hr = PathCchRemoveFileSpec(exe_name, sizeof(exe_name));
    assert(SUCCEEDED(hr));
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(exe_name);
#endif
}

std::string get_absolute_path(char const* const filename)
{
    auto const exe_dir = get_executable_directory();
    return exe_dir + "/" + std::string(filename);
}

std::vector<uint8_t> get_file_contents(char const* const filename)
{
    std::ifstream file(get_absolute_path(filename), std::ios::binary);
    if (!file) {
        return std::vector<uint8_t>();
    }
    file.seekg(0, std::ios::end);
    uint32_t const filesize = static_cast<uint32_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> contents(filesize);
    file.read(reinterpret_cast<char*>(&contents[0]), contents.size());
    return contents;
}

std::pair<int, int> get_window_size(GLFWwindow* const window)
{
    std::pair<int, int> dimensions = {};
    glfwGetFramebufferSize(window, &dimensions.first, &dimensions.second);
    return dimensions;
}

////
// GLFW Callbacks
////
void glfw_error_callback(int const error, char const* description)
{
    std::cout << "GLFW Error: " << error << " - " << description << std::endl;
}
void glfw_keyboard_callback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/,
                            int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}
void glfw_framebuffer_callback(GLFWwindow* window, int width, int height)
{
    std::cout << "Window Resize: (" << width << ", " << height << ")\n";
    get_window_graphics(window)->resize(width, height);
}

}  // namespace

int main(int const /*argc*/, char const* const /*argv*/[])
{
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif  // _MSC_VER
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW Initialization Failed!" << std::endl;
        return 1;
    }
    std::cout << "GLFW initialization succeeded: " << glfwGetVersionString() << "\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window = glfwCreateWindow(800, 600, "Asteroids", nullptr, nullptr);
    glfwSetKeyCallback(window, glfw_keyboard_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_callback);

    //
    // Initialize
    //
    auto graphics = ak::create_graphics(ak::Graphics::kVulkan);
    graphics->create_swap_chain(native_window(window), native_instance());
    glfwSetWindowUserPointer(window, graphics.get());

    // The framebuffer is set here after graphics initialization to trigger the
    // GLFW callback
    set_framebuffer_size(window, kInitialWidth, kInitialHeight);

    //
    // Create resources
    //

    struct Vertex
    {
        float pos[3];
        float col[4];
    };
    Vertex const vertices[] = {
        {
            {-0.7f, -0.7f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f},
        },
        {
            {-0.7f, 0.7f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
        },
        {
            {0.7f, -0.7f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
        },
        {
            {0.7f, 0.7f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f},
        },
    };
    uint16_t const indices[] = {
        0, 1, 2,  //
        2, 3, 1,  //
    };

    auto vertex_buffer = graphics->create_vertex_buffer(sizeof(vertices), vertices);
    auto index_buffer = graphics->create_index_buffer(sizeof(indices), indices);

    std::vector<uint8_t> vs_bytecode;
    std::vector<uint8_t> ps_bytecode;
    switch (graphics->api_type()) {
        case ak::Graphics::kD3D12:
            vs_bytecode = get_file_contents("simple-vs.cso");
            ps_bytecode = get_file_contents("simple-ps.cso");
            break;
        case ak::Graphics::kVulkan:
            vs_bytecode = get_file_contents("simple.vert.spv");
            ps_bytecode = get_file_contents("simple.frag.spv");
            break;
        default:
            break;
    }
    ak::InputLayout const input_layout[] = {
        {
            "POSITION", 0, 3,
        },
        {
            "COLOR", 1, 4,
        },
        ak::kEndLayout,
    };
    auto render_state = graphics->create_render_state({
        {vs_bytecode.data(), vs_bytecode.size()},
        {ps_bytecode.data(), ps_bytecode.size()},
        input_layout,
        "Simple Render State",
    });

    // timing
    int frame_count = 0;
    float fps_elapsed_time = 0.0f;
    uint64_t prev_time = glfwGetTimerValue();

    // Run loop
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();

        //
        // Frame
        //

        // update time
        uint64_t const curr_time = glfwGetTimerValue();
        auto const delta_time = static_cast<float>(curr_time - prev_time) / glfwGetTimerFrequency();
        prev_time = curr_time;
        if (fps_elapsed_time > 0.5f) {
            std::cout << "FPS: " << frame_count * 2 << "\n";
            frame_count = 0;
            fps_elapsed_time -= 0.5f;
        }
        frame_count++;
        fps_elapsed_time += delta_time;

        // render
        struct ConstantBuffer
        {
            mathfu::float4x4 projection;
            mathfu::float4x4 view;
            mathfu::float4x4 world;
        };
        auto* const constant_buffer = graphics->get_upload_data<ConstantBuffer>();
        auto const dimensions = get_window_size(window);
        auto const half_width = dimensions.first / 64.0f;
        auto const half_height = dimensions.second / 64.0f;
        *constant_buffer = {
            mathfu::float4x4::Ortho(-half_width, half_width, -half_height, half_height, 0.0f, 1.0f),
            mathfu::float4x4::Identity(), mathfu::float4x4::Identity(),
        };

        auto* const command_buffer = graphics->command_buffer();
        if (command_buffer != nullptr) {
            command_buffer->begin_render_pass();
            command_buffer->set_render_state(render_state.get());
            command_buffer->set_vertex_buffer(vertex_buffer.get());
            command_buffer->set_index_buffer(index_buffer.get());
            command_buffer->set_constant_data(constant_buffer, sizeof(*constant_buffer));
            command_buffer->draw_indexed(6);
            command_buffer->end_render_pass();
            auto const result = graphics->execute(command_buffer);
            assert(result);
        }

        graphics->present();
    }

    //
    // Shutdown
    //
    graphics->wait_for_idle();

    glfwTerminate();

    return 0;
}
