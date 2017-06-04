#include "application.h"
#include <cassert>
#include <vector>
#include <codecvt>
#include <fstream>

#if defined(_WIN32)
#include <Windows.h>
#include <Pathcch.h>
#endif  // _WIN32

#include "graphics/graphics.h"

namespace {

template<typename T, uint32_t kSize>
constexpr uint32_t array_length(T (&)[kSize])
{
    return kSize;
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

}  // anonymous namespace

Application::Application(void* native_window, void* native_instance)
    : _window(native_window)
    , _instance(native_instance)
    , _graphics(ak::create_graphics(ak::Graphics::kVulkan))
{
    bool const result = _graphics->create_swap_chain(_window, _instance);
    assert(result && "Could not create swap chain");

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
            {-0.5f, 0.5f, 0.5f}, {0.5f, 0.0f, 0.0f, 1.0f},
        },
        {
            {-0.5f, 0.5f, -0.5f}, {0.0f, 0.5f, 0.0f, 1.0f},
        },
        {
            {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 0.5f, 1.0f},
        },
        {
            {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f},
        },

        {
            {-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f},
        },
        {
            {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f},
        },
        {
            {0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f},
        },
        {
            {0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 0.0f, 1.0f},
        },
    };
    uint16_t const indices[] = {
        0, 2, 1,  // top
        2, 0, 3,  //

        1, 2, 6,  // front
        6, 5, 1,  //

        2, 3, 7,  // right
        7, 6, 2,  //

        3, 0, 4,  // back
        4, 7, 3,  //

        0, 1, 5,  // left
        5, 4, 0,  //

        4, 5, 6,  // bottom
        6, 7, 4,  //
    };

    _cube_model.vertex_buffer = _graphics->create_vertex_buffer(sizeof(vertices), vertices);
    _cube_model.index_buffer = _graphics->create_index_buffer(sizeof(indices), indices);
    _cube_model.index_count = array_length(indices);
    _cube_model.vertex_count = array_length(vertices);

    std::vector<uint8_t> vs_bytecode;
    std::vector<uint8_t> ps_bytecode;
    switch (_graphics->api_type()) {
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
    _render_state = _graphics->create_render_state({
        {vs_bytecode.data(), vs_bytecode.size()},
        {ps_bytecode.data(), ps_bytecode.size()},
        input_layout,
        "Simple Render State",
    });

    _constant_buffer = {
        mathfu::float4x4::Identity(), mathfu::float4x4::Identity(), mathfu::float4x4::Identity(),
    };
}

Application::~Application()
{
    _graphics->wait_for_idle();
}

void Application::on_resize(int width, int height)
{
    _width = width;
    _height = height;

    // resize graphics
    _graphics->resize(width, height);

    // update projection
    float const fov = 3.1415926f / 2;
    _constant_buffer.projection =
        mathfu::float4x4::Perspective(fov, width / static_cast<float>(height), 0.1f, 100.0f, -1);
    _constant_buffer.view = mathfu::float4x4::LookAt({0, 0, 0}, {0, 2, -3}, {0, 1, 0}, -1);

    if (_graphics->api_type() == ak::Graphics::kVulkan) {
        // In Vulkan, Y goes down the screen. Flip the projection matrix to account for this
        _constant_buffer.projection(1, 1) *= -1;
    }
}

void Application::on_frame(float const delta_time)
{
    // render
    auto* const vs_const_buffer = _graphics->get_upload_data<VSConstantBuffer>();
    if (vs_const_buffer != nullptr) {
        _constant_buffer.world *=
            mathfu::float4x4::FromRotationMatrix(mathfu::float4x4::RotationY(delta_time));
        *vs_const_buffer = _constant_buffer;
    }

    auto const ps_const_buffer = _graphics->get_upload_data<PSConstantBuffer>();
    if (ps_const_buffer) {
        ps_const_buffer->color = {1, 0, 1, 1};
    }

    auto* const command_buffer = _graphics->command_buffer();
    if (command_buffer != nullptr) {
        command_buffer->begin_render_pass();
        command_buffer->set_render_state(_render_state.get());
        command_buffer->set_vertex_buffer(_cube_model.vertex_buffer.get());
        command_buffer->set_index_buffer(_cube_model.index_buffer.get());
        command_buffer->set_vertex_constant_data(vs_const_buffer, sizeof(*vs_const_buffer));
        command_buffer->set_pixel_constant_data(ps_const_buffer, sizeof(*ps_const_buffer));
        command_buffer->draw_indexed(_cube_model.index_count);
        command_buffer->end_render_pass();
        auto const result = _graphics->execute(command_buffer);
        assert(result);
    }

    _graphics->present();
}
