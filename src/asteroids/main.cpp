#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#include <iostream>
#include <cassert>
#include <vector>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "application.h"

namespace {

constexpr int kInitialWidth = 1920;
constexpr int kInitialHeight = 1080;

bool s_cursor_disabled = false;  // TODO(kw): remove global
mathfu::float2 s_prev_cursor_pos = {};

////
// GLFW helpers
////
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

void* native_instance()
{
#if defined(_WIN32)
    return static_cast<void*>(GetModuleHandle(nullptr));
#elif defined(__APPLE__)
    return nullptr;  // TODO(kw): return NSApp
#else
#warning "Not passing native application"
    return nullptr;
#endif
}

float get_window_scale(GLFWwindow* window)
{
    int windowWidth = 0;
    int framebufferWidth = 0;
    glfwGetWindowSize(window, &windowWidth, nullptr);
    glfwGetFramebufferSize(window, &framebufferWidth, nullptr);
    return static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth);
}
/// @brief Sets a windows framebuffer size. This may be different than the size
///     in "points" on high-DPI displays
void set_framebuffer_size(GLFWwindow* window, int width, int height)
{
    float const scale = get_window_scale(window);
    glfwSetWindowSize(window, static_cast<int>(width / scale), static_cast<int>(height / scale));
}
Application* get_window_application(GLFWwindow* window)
{
    return static_cast<Application*>(glfwGetWindowUserPointer(window));
}

////
// GLFW Callbacks
////
void glfw_error_callback(int const error, char const* description)
{
    std::cout << "GLFW Error: " << error << " - " << description << std::endl;
}
void glfw_keyboard_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
    if (action == GLFW_RELEASE) {
        get_window_application(window)->on_keyup(key);
    }
}
void glfw_framebuffer_callback(GLFWwindow* window, int width, int height)
{
    std::cout << "Window Resize: (" << width << ", " << height << ")\n";
    get_window_application(window)->on_resize(width, height);
}
void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            s_cursor_disabled = true;
        } else if (action == GLFW_RELEASE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            s_cursor_disabled = false;
        }
    }
}
void glfw_mouse_pos_callback(GLFWwindow* window, double x, double y)
{
    mathfu::float2 const curr_cursor_pos(static_cast<float>(x), static_cast<float>(y));
    auto const delta = curr_cursor_pos - s_prev_cursor_pos;
    if (s_cursor_disabled) {
        get_window_application(window)->on_mouse_move(delta.x, delta.y);
    }
    s_prev_cursor_pos = curr_cursor_pos;
}
void glfw_mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    get_window_application(window)->on_scroll(static_cast<float>(yoffset));
}

}  // namespace

int main(int const /*argc*/, char const* const /*argv*/[])
{
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif  // _MSC_VER
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW Initialization Failed!" << std::endl;
        return 1;
    }
    std::cout << "GLFW initialization succeeded: " << glfwGetVersionString() << "\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window = glfwCreateWindow(800, 600, "Asteroids", nullptr, nullptr);
    glfwSetKeyCallback(window, glfw_keyboard_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_pos_callback);
    glfwSetScrollCallback(window, glfw_mouse_scroll_callback);

    //
    // Initialize
    //
    auto app = std::make_unique<Application>(native_window(window), native_instance());
    glfwSetWindowUserPointer(window, app.get());

    // The framebuffer is set here after graphics initialization to trigger the
    // GLFW callback
    set_framebuffer_size(window, kInitialWidth, kInitialHeight);

    // timing
    int frame_count = 0;
    float fps_elapsed_time = 0.0f;
    uint64_t prev_time = glfwGetTimerValue();

    // Run loop
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();

        // update time
        uint64_t const curr_time = glfwGetTimerValue();
        auto const delta_time = static_cast<float>(curr_time - prev_time) / glfwGetTimerFrequency();
        prev_time = curr_time;
        if (fps_elapsed_time > 0.5f) {
            std::cout << "FPS: " << frame_count * 2 << "\n";
            frame_count = 0;
            fps_elapsed_time -= 0.5f;
        }
        frame_count++;
        fps_elapsed_time += delta_time;

        //
        // Frame
        //
        app->on_frame(delta_time);
    }

    //
    // Shutdown
    //
    glfwTerminate();

    return 0;
}
