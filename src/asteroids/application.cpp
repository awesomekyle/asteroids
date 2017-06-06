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

#include <GLFW/glfw3.h>

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
#else
    // TODO
    return std::string();
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
    file.read(reinterpret_cast<char*>(&contents[0]),
              static_cast<std::streamsize>(contents.size()));
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
    void calculate_normals()
    {
        for (auto& v : vertices) {
            v.norm = {0, 0, 0};
        }

        assert(indices.size() % 3 == 0);  // trilist
        for (size_t ii = 0; ii < indices.size(); ii += 3) {
            auto& v1 = vertices[indices[ii + 0]];
            auto& v2 = vertices[indices[ii + 1]];
            auto& v3 = vertices[indices[ii + 2]];

            // Two edge vectors u,v
            auto const u = v2.pos - v1.pos;
            auto const v = v3.pos - v1.pos;

            // cross(u,v)
            auto const n = mathfu::float3::CrossProduct(u, v);

            // Add them all together
            v1.norm += n;
            v2.norm += n;
            v3.norm += n;
        }

        // Normalize
        for (auto& v : vertices) {
            v.norm.Normalize();
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
    , _graphics(ak::create_graphics())
{
    bool const result = _graphics->create_swap_chain(_window, _instance);
    assert(result && "Could not create swap chain");

    //
    // Create resources
    //
    auto mesh = Mesh::icosahedron();
    mesh.spherify(1.0f);
    mesh.calculate_normals();
    auto const index_count = static_cast<uint32_t>(mesh.indices.size());
    auto const vertex_count = static_cast<uint32_t>(mesh.vertices.size());
    _cube_model.vertex_buffer =
        _graphics->create_vertex_buffer(vertex_count * sizeof(mesh.vertices[0]),
                                        mesh.vertices.data());
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
        case ak::Graphics::kMetal:
            // TODO: Load shaders (from DefaultLibrary or as file like Vk/D3D?)
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
        mathfu::float4x4::Identity(), mathfu::float4x4::Identity(),
    };

    // initialize asteroids
    std::random_device rd;
    std::mt19937 gen(rd());

    constexpr float sim_disc_radius = 120.0f;
    std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f);
    std::uniform_real_distribution<float> orbit_dist(sim_disc_radius * 0.6f, 450.0f);
    std::uniform_real_distribution<float> radial_velocity_dist(5.0f, 15.0f);
    std::uniform_real_distribution<float> height_dist(-0.2f, 0.2f);
    std::uniform_real_distribution<float> angle_dist(-3.14159265358979323846f,
                                                     3.14159265358979323846f);

    for (auto& asteroid : _asteroids) {
        auto const scale = scale_dist(gen);
        auto const orbit_radius = orbit_dist(gen);
        auto const position_angle = angle_dist(gen);
        auto const height = sim_disc_radius * height_dist(gen);

        asteroid.scale = scale;
        asteroid.orbit_velocity = radial_velocity_dist(gen) / (scale * orbit_radius);

        asteroid.world =
            mathfu::float4x4::FromRotationMatrix(mathfu::float4x4::RotationY(position_angle)) *
            mathfu::float4x4::FromTranslationVector({orbit_radius, height, 0.0f}) *
            mathfu::float4x4::FromScaleVector({scale, scale, scale});
    }
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

    // to get better depth buffer precision, a "reversed" depth buffer is used
    // https://developer.nvidia.com/content/depth-precision-visualized
    _constant_buffer.projection =
        mathfu::float4x4::Perspective(fov, width / static_cast<float>(height), 10000.0f, 0.1f, -1);
    _constant_buffer.view = mathfu::float4x4::LookAt({0, 0, 0}, {0, 70, -500}, {0, 1, 0}, -1);

    if (_graphics->api_type() == ak::Graphics::kVulkan) {
        // In Vulkan, Y goes down the screen. Flip the projection matrix to account for this
        _constant_buffer.projection(1, 1) *= -1;
    }

    _constant_buffer.viewproj = _constant_buffer.projection * _constant_buffer.view;
}

void Application::on_frame(float const delta_time)
{
    // update
    if (_simulate) {
        for (auto& asteroid : _asteroids) {
            auto const orbit = mathfu::float4x4::FromRotationMatrix(
                mathfu::float4x4::RotationY(asteroid.orbit_velocity * delta_time));
            asteroid.world = orbit * asteroid.world;
            asteroid.wvp = _constant_buffer.viewproj * asteroid.world;
        }
    }

    // render
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
        command_buffer->set_pixel_constant_data(ps_const_buffer, sizeof(*ps_const_buffer));

        // set per-frame constants
        auto* const vs_const_buffer = _graphics->get_upload_data<PerFrameConstants>();
        if (vs_const_buffer != nullptr) {
            *vs_const_buffer = _constant_buffer;
        }
        command_buffer->set_vertex_constant_data(0, vs_const_buffer, sizeof(*vs_const_buffer));

        for (auto& asteroid : _asteroids) {
            auto* const model_buffer = _graphics->get_upload_data<PerModelConstants>();
            if (model_buffer != nullptr) {
                model_buffer->wvp = asteroid.wvp;
                model_buffer->world = asteroid.world;
            }

            command_buffer->set_vertex_constant_data(1, model_buffer, sizeof(*model_buffer));
            command_buffer->draw_indexed(_cube_model.index_count);
        }

        command_buffer->end_render_pass();
        auto const result = _graphics->execute(command_buffer);
        assert(result);
    }

    _graphics->present();
}

void Application::on_keyup(int const glfw_key)
{
    if (glfw_key == GLFW_KEY_SPACE) {
        _simulate = !_simulate;
    }
}
