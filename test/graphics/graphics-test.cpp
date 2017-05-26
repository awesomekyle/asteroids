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

void* NativeWindow(GLFWwindow* const window)
{
    [[gsl::suppress(lifetime)]]
#if defined(_WIN32)
    return glfwGetWin32Window(window);
#elif defined(__APPLE__)
    return glfwGetCocoaWindow(window);
#else
#warning "Not passing native window to Gfx"
    return nullptr;
#endif
}

void* NativeInstance(void)
{
#if defined(_WIN32)
    [[gsl::suppress(lifetime)]]
    return static_cast<void*>(GetModuleHandle(nullptr));
#else
#warning "Not passing native application"
    return nullptr;
#endif
}

TEST_CASE("graphics lifetime")
{
    SECTION("graphics device creation") {
        WHEN("a graphics device is created") {
            auto graphicsDevice = ak::create_graphics();
            THEN("the device is successfully created") {
                REQUIRE(graphicsDevice);
            }
        }
    }
}

TEST_CASE("Window interaction")
{
    GIVEN("A Graphics object and OS window") {
        REQUIRE(glfwInitialized);
        REQUIRE(glfwTerminationRegistered == 0);

        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
        REQUIRE(window);

        // Gfx initialization
        auto graphics = ak::create_graphics();
        REQUIRE(graphics);

        WHEN("presented with no swap chain") {
            bool const result = graphics->present();
            THEN("the resize fails") {
                REQUIRE_FALSE(result);
            }
        }
        WHEN("resized with no swap chain") {
            bool const result = graphics->resize(10, 10);
            THEN("the resize fails") {
                REQUIRE_FALSE(result);
            }
        }
        WHEN("a swap chain is created") {
            bool const result = graphics->create_swap_chain(NativeWindow(window),
                                                            NativeInstance());
            THEN("the creation was successful") {
                REQUIRE(result);
            }
            THEN("the swap chain can be resized") {
                REQUIRE(graphics->resize(10, 10));
            }
            THEN("the swap chain can be presented") {
                REQUIRE(graphics->present());
            }
        }

        glfwDestroyWindow(window);
    }
}

} // anonymous

