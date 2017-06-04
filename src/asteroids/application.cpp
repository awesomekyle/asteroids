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
        0, 1, 2,  //
        2, 3, 0,  //
    };

    _vertex_buffer = _graphics->create_vertex_buffer(sizeof(vertices), vertices);
    _index_buffer = _graphics->create_index_buffer(sizeof(indices), indices);

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
        mathfu::float4x4::Perspective(fov, width / static_cast<float>(height), 0.1f, 100.0f);
    _constant_buffer.view = mathfu::float4x4::LookAt({0, 0, 0}, {0, 2, 3}, {0, 1, 0}, 1);

    if (_graphics->api_type() == ak::Graphics::kVulkan) {
        // In Vulkan, Y goes down the screen. Flip the projection matrix to account for this
        _constant_buffer.projection(1, 1) *= -1;
    }
}

void Application::on_frame(float /*delta_time*/)
{
    // render
    auto* const constant_buffer = _graphics->get_upload_data<ConstantBuffer>();
    if (constant_buffer != nullptr) {
        *constant_buffer = _constant_buffer;
    }

    auto* const command_buffer = _graphics->command_buffer();
    if (command_buffer != nullptr) {
        command_buffer->begin_render_pass();
        command_buffer->set_render_state(_render_state.get());
        command_buffer->set_vertex_buffer(_vertex_buffer.get());
        command_buffer->set_index_buffer(_index_buffer.get());
        command_buffer->set_constant_data(constant_buffer, sizeof(*constant_buffer));
        command_buffer->draw_indexed(6);
        command_buffer->end_render_pass();
        auto const result = _graphics->execute(command_buffer);
        assert(result);
    }

    _graphics->present();
}
