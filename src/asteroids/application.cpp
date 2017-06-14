#include "application.h"
#include <cassert>
#include <vector>
#include <codecvt>
#include <fstream>
#include <map>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)  // double -> float conversion
#endif                           // _MSC_VER

#include <random>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // _MSC_VER

#if defined(_WIN32)
#include <Windows.h>
#include <Pathcch.h>
#endif  // _WIN32

#include <GLFW/glfw3.h>

#include "graphics/graphics.h"
#include "noise.h"

namespace {

constexpr float kMinScale = 0.2f;
constexpr float kPi = 3.14159265358979323846f;

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
    file.read(reinterpret_cast<char*>(&contents[0]), static_cast<std::streamsize>(contents.size()));
    return contents;
}

//
// Meshes
//
struct Mesh
{
    using IndexType = uint16_t;

    struct Edge
    {
        Edge(IndexType i0, IndexType i1)
            : v0(i0)
            , v1(i1)
        {
            if (v0 > v1) {
                std::swap(v0, v1);
            }
        }
        IndexType v0;
        IndexType v1;

        bool operator<(const Edge& c) const { return v0 < c.v0 || (v0 == c.v0 && v1 < c.v1); }
    };

    struct Vertex
    {
        mathfu::float3 pos;
        mathfu::float3 norm;
    };

    std::vector<Vertex> vertices;
    std::vector<IndexType> indices;

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
    void bumpify()
    {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> random_noise(0.0f, 10000.0f);
        std::normal_distribution<float> random_persistence(0.95f, 0.04f);
        float const noise_scale = 0.5f;
        float const radius_scale = 0.9f;
        float const radius_bias = 0.3f;

        NoiseOctaves<4> const texture_noise(random_persistence(gen));
        float const noise = random_noise(gen);

        for (auto& v : vertices) {
            float radius = texture_noise(v.pos.x * noise_scale, v.pos.y * noise_scale,
                                         v.pos.z * noise_scale, noise);
            radius = radius * radius_scale + radius_bias;
            v.pos *= radius;
        }
    }
    void subdivide()
    {
        std::map<Edge, IndexType> midpoints;
        auto const add_midpoint = [&](Edge const& edge) -> IndexType {
            auto const found = midpoints.find(edge);
            if (found != midpoints.end()) {
                return found->second;
            }

            auto const& p0 = vertices[edge.v0].pos;
            auto const& p1 = vertices[edge.v1].pos;

            Vertex const mid = {{
                (p0 + p1) * 0.5f,
            }};

            IndexType const new_index = static_cast<IndexType>(vertices.size());
            vertices.push_back(mid);
            midpoints[edge] = new_index;
            return new_index;
        };

        std::vector<IndexType> new_indices;
        new_indices.reserve(indices.size());

        for (size_t ii = 0; ii < indices.size(); ii += 3) {
            auto& i0 = indices[ii + 0];
            auto& i1 = indices[ii + 1];
            auto& i2 = indices[ii + 2];

            auto const m0 = add_midpoint(Edge(i0, i1));
            auto const m1 = add_midpoint(Edge(i1, i2));
            auto const m2 = add_midpoint(Edge(i2, i0));

            IndexType const updated_indices[] = {
                i0, m0, m2, m0, i1, m1, m0, m1, m2, m2, m1, i2,
            };
            new_indices.insert(new_indices.end(), updated_indices,
                               updated_indices + array_length(updated_indices));
        }

        std::swap(indices, new_indices);
    }
    void calculate_normals()
    {
        for (auto& v : vertices) {
            v.norm = {0, 0, 0};
        }

        assert(indices.size() % 3 == 0);  // trilist
        for (size_t ii = 0; ii < indices.size(); ii += 3) {
            auto& v0 = vertices[indices[ii + 0]];
            auto& v1 = vertices[indices[ii + 1]];
            auto& v2 = vertices[indices[ii + 2]];

            // Two edge vectors u,v
            auto const u = v1.pos - v0.pos;
            auto const v = v2.pos - v0.pos;

            // cross(u,v)
            auto const n = mathfu::float3::CrossProduct(u, v);

            // Add them all together
            v0.norm += n;
            v1.norm += n;
            v2.norm += n;
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
    , _graphics(ak::create_graphics(ak::Graphics::kVulkan))
{
    bool const result = _graphics->create_swap_chain(_window, _instance);
    assert(result && "Could not create swap chain");

    //
    // Create resources
    //
    auto mesh = Mesh::icosahedron();
    mesh.subdivide();
    mesh.subdivide();
    mesh.spherify(1.0f);
    mesh.bumpify();
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

    std::normal_distribution<float> scale_dist(1.3f, 0.7f);
    std::normal_distribution<float> orbit_dist(kSimOrbitRadius, kSimDiscRadius * 0.6f);
    std::normal_distribution<float> height_dist(0.0f, 0.4f);
    std::uniform_real_distribution<float> radial_velocity_dist(5.0f, 15.0f);
    std::uniform_real_distribution<float> angle_dist(-kPi, kPi);

    for (auto& asteroid : _asteroids) {
        auto const scale = std::max(scale_dist(gen), kMinScale);
        auto const orbit_radius = orbit_dist(gen);
        auto const position_angle = angle_dist(gen);
        auto const height = kSimDiscRadius * height_dist(gen);

        asteroid.scale = scale;
        asteroid.orbit_velocity = -radial_velocity_dist(gen) / (scale * orbit_radius);
        assert(asteroid.orbit_velocity < 0.0f);

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
    float const fov = kPi / 2;

    // to get better depth buffer precision, a "reversed" depth buffer is used
    // https://developer.nvidia.com/content/depth-precision-visualized
    _constant_buffer.projection =
        mathfu::float4x4::Perspective(fov, width / static_cast<float>(height), 10000.0f, 0.1f, -1);

    if (_graphics->api_type() == ak::Graphics::kVulkan) {
        // In Vulkan, Y goes down the screen. Flip the projection matrix to account for this
        _constant_buffer.projection(1, 1) *= -1;
    }

    recalculate_camera();
}

void Application::on_frame(float const delta_time)
{
    // camera
    constexpr float min_radius = kSimOrbitRadius - 3.0f * kSimDiscRadius;
    _cam_radius = std::max(min_radius, _cam_radius + _wheel_delta * delta_time * -300.0f);
    _wheel_delta = 0.0f;

    _cam_long_angle += _cursor_delta.x * -delta_time;

    float const limit = kPi * 0.00625f;
    _cam_lat_angle =
        std::max(limit, std::min(kPi - limit, _cam_lat_angle + _cursor_delta.y * -delta_time));

    recalculate_camera();
    _cursor_delta = {0, 0};

    // update
    if (_simulate) {
        for (auto& asteroid : _asteroids) {
            auto const orbit = mathfu::float4x4::FromRotationMatrix(
                mathfu::float4x4::RotationY(asteroid.orbit_velocity * delta_time));
            asteroid.world = orbit * asteroid.world;
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

void Application::on_mouse_move(float delta_x, float delta_y)
{
    _cam_long_angle += delta_x * 0.0001f;

    float const limit = kPi * 0.01f;
    _cam_lat_angle = std::max(limit, std::min(kPi - limit, _cam_lat_angle + delta_y * 0.0001f));

    _cursor_delta = {delta_x, delta_y};

    recalculate_camera();
}

void Application::on_scroll(float scroll)
{
    _wheel_delta = scroll;
}

void Application::recalculate_camera()
{
    mathfu::float3 const center(0.0f, -0.4f * kSimDiscRadius, 0.0f);
    mathfu::float3 const position(_cam_radius * std::sin(_cam_lat_angle) *
                                      std::cos(_cam_long_angle),
                                  _cam_radius * std::cos(_cam_lat_angle),
                                  _cam_radius * std::sin(_cam_lat_angle) *
                                      std::sin(_cam_long_angle));
    _constant_buffer.view = mathfu::float4x4::LookAt(center, position, {0, 1, 0}, -1);

    _constant_buffer.viewproj = _constant_buffer.projection * _constant_buffer.view;
}
