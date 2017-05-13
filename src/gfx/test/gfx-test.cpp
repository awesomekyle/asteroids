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

// Global initialization
int const glfwInitialized = glfwInit();
int const glfwTerminationRegistered = atexit(glfwTerminate);

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
        REQUIRE(glfwInitialized);
        REQUIRE(glfwTerminationRegistered == 0);

        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* const window = glfwCreateWindow(10, 10, "Gfx Test", NULL, NULL);
        REQUIRE(window);
        // Gfx initialization
        Gfx* const G = gfxCreate();
        REQUIRE(G);

        WHEN("resized with no swap chain")
        {
            bool const result = gfxResize(G, 10, 10);
            THEN("the resize fails")
            {
                REQUIRE_FALSE(result);
            }
        }
        WHEN("a swap chain is created")
        {
            bool const result = gfxCreateSwapChain(G, NativeWindow(window));
            THEN("the creation was successful")
            {
                REQUIRE(result);
            }
            THEN("the device can be resized")
            {
                REQUIRE(gfxResize(G, 10, 10));
            }
            THEN("back buffers can be obtained")
            {
                REQUIRE(gfxGetBackBuffer(G) != kGfxInvalidHandle);
            }
        }

        glfwDestroyWindow(window);
        gfxDestroy(G);
    }
}
TEST_CASE("Gfx command interface")
{
    GIVEN("A graphics object")
    {
        // Gfx initialization
        Gfx* const G = gfxCreate();
        REQUIRE(G);

        WHEN("a command buffer is requested")
        {
            GfxCmdBuffer* const cmdBuffer = gfxGetCommandBuffer(G);
            THEN("a valid command buffer is returned")
            {
                REQUIRE(cmdBuffer);
                REQUIRE(gfxNumAvailableCommandBuffers(G) == 127);

                AND_WHEN("the command buffer is released")
                {
                    gfxResetCommandBuffer(cmdBuffer);

                    THEN("the buffer is added back to the pool")
                    {
                        REQUIRE(gfxNumAvailableCommandBuffers(G) == 128);
                    }
                }
            }
            THEN("it can be properly executed") {
                REQUIRE(gfxExecuteCommandBuffer(cmdBuffer));
            }
            THEN("render passes can be executed") {
                float const clearColor[] = { 0, 0, 0, 1 };
                gfxCmdBeginRenderPass(cmdBuffer,
                                      kGfxInvalidHandle,
                                      kGfxRenderPassActionClear,
                                      clearColor);
                gfxCmdEndRenderPass(cmdBuffer);
            }
        }
        WHEN("all command buffers are requested")
        {
            for (size_t ii = 0; ii < 128; ii++) {
                gfxGetCommandBuffer(G);
            }
            THEN("requesting another fails") {
                REQUIRE(gfxGetCommandBuffer(G) == nullptr);
            }
        }

        gfxDestroy(G);
    }
}

} // anonymous
