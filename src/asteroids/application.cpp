#include "application.h"
#include <cassert>
#include <vector>
#include <codecvt>
#include <fstream>
#include <random>

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

//
// Meshes
//
struct Mesh
{
    struct Vertex
    {
        mathfu::float3 pos;
        mathfu::float3 norm;
    };

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    void spherify(float const radius)
    {
        for (auto& v : vertices) {
            float const n =
                radius / std::sqrt(v.pos.x * v.pos.x + v.pos.y * v.pos.y + v.pos.z * v.pos.z);
            v.pos.x *= n;
            v.pos.y *= n;
            v.pos.z *= n;
        }
    }

    static Mesh icosahedron()
    {
        float const a = std::sqrt(2.0f / (5.0f - std::sqrt(5.0f)));
        float const b = std::sqrt(2.0f / (5.0f + std::sqrt(5.0f)));
        // clang-format off
        Mesh const mesh = {
            {
                { {-b,  a,  0}, },
                { { b,  a,  0}, },
                { {-b, -a,  0}, },
                { { b, -a,  0}, },
                { { 0, -b,  a}, },
                { { 0,  b,  a}, },
                { { 0, -b, -a}, },
                { { 0,  b, -a}, },
                { { a,  0, -b}, },
                { { a,  0,  b}, },
                { {-a,  0, -b}, },
                { {-a,  0,  b}, },
            },
            {
                 0,  5, 11,
                 0,  1,  5,
                 0,  7,  1,
                 0, 10,  7,
                 0, 11, 10,
                 1,  9,  5,
                 5,  4, 11,
                11,  2, 10,
                10,  6,  7,
                 7,  8,  1,
                 3,  4,  9,
                 3,  2,  4,
                 3,  6,  2,
                 3,  8,  6,
                 3,  9,  8,
                 4,  5,  9,
                 2, 11,  4,
                 6, 10,  2,
                 8,  7,  6,
                 9,  1,  8,
            },
        };
        // clang-format on
        return mesh;
    }
    static Mesh cube()
    {
        // clang-format off
        Mesh const mesh = {
            {
                { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f } },
                { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f } },
                { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f } },
                { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f } },

                { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f } },
                { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f } },
                { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f } },
                { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f } },

                { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f } },
                { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f } },
                { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f } },
                { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f } },

                { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f } },
                { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f } },
                { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f } },
                { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f } },

                { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
                { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
                { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
                { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },

                { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
                { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
                { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
                { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
            },
            {
                3,1,0,
                2,1,3,

                6,4,5,
                7,4,6,

                11,9,8,
                10,9,11,

                14,12,13,
                15,12,14,

                19,17,16,
                18,17,19,

                22,20,21,
                23,20,22,
            },
        };
        // clang-format on
        return mesh;
    }
};

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
    auto mesh = Mesh::cube();
    // mesh.spherify(1.0f);
    std::random_device rd;
    std::mt19937 gen(rd());
    // for (auto& vertex : mesh.vertices) {
    //    vertex.col = {
    //        std::uniform_real_distribution<float>(0.25f, 1.0f)(gen),
    //        std::uniform_real_distribution<float>(0.25f, 1.0f)(gen),
    //        std::uniform_real_distribution<float>(0.25f, 1.0f)(gen),
    //    };
    //}
    auto const index_count = static_cast<uint32_t>(mesh.indices.size());
    auto const vertex_count = static_cast<uint32_t>(mesh.vertices.size());
    _cube_model.vertex_buffer =
        _graphics->create_vertex_buffer(vertex_count * sizeof(mesh.vertices[0]),
                                        mesh.vertices.data());
    _cube_model.vertex_count = vertex_count;
    _cube_model.index_buffer =
        _graphics->create_index_buffer(index_count * sizeof(mesh.indices[0]), mesh.indices.data());
    _cube_model.index_count = index_count;

    // Render state
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
            "COLOR", 1, 3,
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
        ps_const_buffer->color = {1, 1, 1, 1};
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
