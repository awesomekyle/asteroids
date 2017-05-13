#include "catch.hpp"
#if defined(_WIN32)
#   define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#   define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#   define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
extern "C" {
#include "gfx/gfx.h"
}

namespace {

void* NativeWindow(GLFWwindow* const window)
{
#if defined(_WIN32)
    return glfwGetWin32Window(window);
#else
    #warning "Not passing native window to Gfx"
    return nullptr;
#endif
}

TEST_CASE("Gfx lifetime")
{
    SECTION("Gfx can be created and destroyed")
    {
        Gfx* const G = gfxCreate();
        REQUIRE(G);
        gfxDestroy(G);
    }
}
TEST_CASE("Gfx window interaction")
{
    GIVEN("A Graphics object and OS window")
    {
        // GLFW initialization
        REQUIRE(glfwInit());
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
        REQUIRE(window);
        // Gfx initialization
        Gfx* const G = gfxCreate();
        REQUIRE(G);

        WHEN("A swap chain is created")
        {
            bool const result = gfxCreateSwapChain(G, NativeWindow(window));
            THEN("the creation was successful")
            {
                REQUIRE(result);
            }
        }

        gfxDestroy(G);
        glfwTerminate();
    }
}

} // anonymous
