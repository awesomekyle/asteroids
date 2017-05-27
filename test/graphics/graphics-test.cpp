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
#else
#warning "Not passing native application"
    return nullptr;
#endif
}

TEST_CASE("graphics lifetime")
{
    SECTION("graphics device creation")
    {
        WHEN("a graphics device is created")
        {
            auto graphicsDevice = ak::create_graphics();
            THEN("the device is successfully created") { REQUIRE(graphicsDevice); }
        }
    }
}

TEST_CASE("Window interaction")
{
    GIVEN("A Graphics object and OS window")
    {
        REQUIRE(glfwInitialized);
        REQUIRE(glfwTerminationRegistered == 0);

        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
        REQUIRE(window);

        // Gfx initialization
        auto graphics = ak::create_graphics();
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
    GIVEN("A Graphics object and OS window")
    {
        // Gfx initialization
        auto graphics = ak::create_graphics();
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
            // can run out
            // render passes can be created if swap chain exists
            // render passes fail with no swap chain
        }
    }
}

}  // anonymous namespace
