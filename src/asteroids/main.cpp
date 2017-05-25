#include <iostream>
#include <gsl\span>
#include <GLFW/glfw3.h>

namespace {

constexpr int kInitialWidth = 1920;
constexpr int kInitialHeight = 1080;

////
// GLFW Callbacks
////
void glw_error_callback(int const error, char const* description)
{
    std::cout << "GLW Error: " << error << " - " <<  description << std::endl;
}

} // anonymous

int main(int const /*argc*/, char const* const /*argv*/[])
{
    // Initialize GLFW
    glfwSetErrorCallback(::glw_error_callback);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW Initialization Failed!" << std::endl;
        return 1;
    }
    std::cout << "GLFW initialization succeeded: " << glfwGetVersionString() << "\n";
    return 0;
}
