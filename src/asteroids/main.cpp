#include <iostream>
#include <gsl\span>
#include <GLFW/glfw3.h>

namespace {

constexpr int kInitialWidth = 1920;
constexpr int kInitialHeight = 1080;

////
// GLFW helpers
////
float get_window_scale(GLFWwindow* window)
{
    int windowWidth = 0;
    int framebufferWidth = 0;
    glfwGetWindowSize(window, &windowWidth, NULL);
    glfwGetFramebufferSize(window, &framebufferWidth, NULL);
    return framebufferWidth / (float)windowWidth;
}
/// @brief Sets a windows framebuffer size. This may be different than the size
///     in "points" on high-DPI displays
void set_framebuffer_size(GLFWwindow* window, int width, int height)
{
    float const scale = get_window_scale(window);
    glfwSetWindowSize(window,
                      static_cast<int>(width / scale),
                      static_cast<int>(height / scale));
}

////
// GLFW Callbacks
////
void glfw_error_callback(int const error, char const* description)
{
    std::cout << "GLFW Error: " << error << " - " <<  description << std::endl;
}
void glfw_keyboard_callback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}
static void glfw_framebuffer_callback(GLFWwindow* /*window*/, int width, int height)
{
    std::cout << "Window Resize: (" << width << ", " << height << ")\n";
}

} // anonymous

int main(int const /*argc*/, char const* const /*argv*/[])
{
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW Initialization Failed!" << std::endl;
        return 1;
    }
    std::cout << "GLFW initialization succeeded: " << glfwGetVersionString() << "\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* const window = glfwCreateWindow(800, 600, "Asteroids", nullptr, nullptr);
    glfwSetKeyCallback(window, glfw_keyboard_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_callback);

    // The framebuffer is set here after graphics initialization to trigger the
    // GLFW callback
    set_framebuffer_size(window, kInitialWidth, kInitialHeight);

    // Run loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    glfwTerminate();

    return 0;
}
