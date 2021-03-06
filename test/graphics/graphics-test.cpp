#include "catch.hpp"

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <gsl/gsl>

#include "graphics/graphics.h"

namespace {

// Global initialization
int const glfwInitialized = glfwInit();
int const glfwTerminationRegistered = atexit(glfwTerminate);

constexpr ak::Graphics::API kTestApi = ak::Graphics::kVulkan;

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

void* native_instance(void)
{
#if defined(_WIN32)
    return static_cast<void*>(GetModuleHandle(nullptr));
#elif defined(__APPLE__)
    return nullptr;  // TODO: Get NSApp
#else
#warning "Not passing native application"
    return nullptr;
#endif
}

TEST_CASE("graphics lifetime")
{
    SECTION("graphics device creation")
    {
        WHEN("the default platofmr device is requiested")
        {
            auto graphicsDevice = ak::create_graphics();
            THEN("the device is successfully created")
            {
                REQUIRE(graphicsDevice);
                REQUIRE(graphicsDevice->api_type() != ak::Graphics::kDefault);
            }
        }
        WHEN("a graphics device is created")
        {
            auto graphicsDevice = ak::create_graphics(kTestApi);
            THEN("the device is successfully created") { REQUIRE(graphicsDevice); }
        }
    }
}

TEST_CASE("Window interaction")
{
    GIVEN("a graphics device and OS window")
    {
        REQUIRE(glfwInitialized);
        REQUIRE(glfwTerminationRegistered == 0);

        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
        REQUIRE(window);

        // Gfx initialization
        auto graphics = ak::create_graphics(kTestApi);
        REQUIRE(graphics);

        WHEN("presented with no swap chain")
        {
            bool const result = graphics->present();
            THEN("the resize fails") { REQUIRE_FALSE(result); }
        }
        WHEN("resized with no swap chain")
        {
            bool const result = graphics->resize(10, 10);
            THEN("the resize fails") { REQUIRE_FALSE(result); }
        }
        WHEN("a swap chain is created")
        {
            bool const result =
                graphics->create_swap_chain(native_window(window), native_instance());
            THEN("the creation was successful") { REQUIRE(result); }
            THEN("the swap chain can be resized") { REQUIRE(graphics->resize(10, 10)); }
            THEN("the swap chain can be presented") { REQUIRE(graphics->present()); }
        }

        glfwDestroyWindow(window);
    }
}
TEST_CASE("graphics command interface")
{
    GIVEN("a graphics device")
    {
        auto graphics = ak::create_graphics(kTestApi);
        REQUIRE(graphics);

        WHEN("a command buffer is requested")
        {
            auto* const command_buffer = graphics->command_buffer();
            THEN("a valid command buffer is returned")
            {
                REQUIRE(command_buffer);
                REQUIRE(graphics->num_available_command_buffers() ==
                        ak::Graphics::kMaxCommandBuffers - 1);

                AND_WHEN("the command buffer is released")
                {
                    command_buffer->reset();

                    THEN("the buffer is added back to the pool")
                    {
                        REQUIRE(graphics->num_available_command_buffers() ==
                                ak::Graphics::kMaxCommandBuffers);
                    }
                }
            }
            THEN("it can be properly executed") { REQUIRE(graphics->execute(command_buffer)); }
        }
        WHEN("all command buffers are requested")
        {
            for (size_t ii = 0; ii < ak::Graphics::kMaxCommandBuffers; ii++) {
                graphics->command_buffer();
            }
            THEN("requesting another fails") { REQUIRE(graphics->command_buffer() == nullptr); }
        }
    }
    GIVEN("a command buffer")
    {
        auto graphics = ak::create_graphics(kTestApi);
        REQUIRE(graphics);

        auto* const command_buffer = graphics->command_buffer();
        REQUIRE(command_buffer);

        // TODO: Support render passes using custom render targets
        WHEN("a render pass is begun with no swap chain")
        {
            auto const result = command_buffer->begin_render_pass();
            THEN("the render pass is not created") { REQUIRE_FALSE(result); }
        }
        WHEN("a swap chain is created")
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
            REQUIRE(window);
            REQUIRE(graphics->create_swap_chain(native_window(window), native_instance()));

            THEN("a render pass can be begun")
            {
                REQUIRE(command_buffer->begin_render_pass());
                AND_THEN("the render pass can be ended") { command_buffer->end_render_pass(); }
            }

            // TODO(kw): end with no begin

            glfwDestroyWindow(window);
        }
    }
}

TEST_CASE("graphics resources")
{
    GIVEN("a graphics device")
    {
        auto graphics = ak::create_graphics(kTestApi);
        REQUIRE(graphics);

#if 0  // TODO(kw): When using binary shaders, how do we test this?
        WHEN("a render state is created")
        {
            auto const default_vertex_source = [&]() -> char const* {
                if (graphics->api_type() == ak::Graphics::kD3D12) {
                    return "float4 main(uint vertexId : SV_VertexID) : SV_POSITION {"
                           "    float2 pos[3] = { float2(-0.7, 0.7), float2(0.7, 0.7), float2(0.0, "
                           "-0.7) };"
                           "    return float4(pos[vertexId], 0.0f, 1.0f);"
                           "}";
                }
                return nullptr;
            };
            auto const default_pixel_source = [&]() -> char const* {
                if (graphics->api_type() == ak::Graphics::kD3D12) {
                    return "float4 main() : SV_TARGET {"
                           "    return float4(0.3f, 0.5f, 0.7f, 1.0f);"
                           "}";
                }
                return nullptr;
            };

            auto* const vertex_source = default_vertex_source();
            auto* const pixel_source = default_pixel_source();

            ak::RenderStateDesc const desc = {
                {
                    vertex_source, "main",
                },
                {
                    pixel_source, "main",
                },
                "Simple State",
            };

            auto const render_state = graphics->create_render_state(desc);

            THEN("a valid render state is returned") { REQUIRE(render_state); }
        }
#endif
        WHEN("a vertex buffer is created")
        {
            float const data[] = {
                0.0f, 1.0f, 2.0f, 3.0f,
            };
            auto vertex_buffer = graphics->create_vertex_buffer(sizeof(data), data);
            THEN("a valid buffer is returned") { REQUIRE(vertex_buffer); }
        }
        WHEN("an index buffer is created")
        {
            size_t const data[] = {
                0, 1, 2, 3,
            };
            auto index_buffer = graphics->create_index_buffer(sizeof(data), data);
            THEN("a valid buffer is returned") { REQUIRE(index_buffer); }
        }
    }
}

}  // anonymous namespace
