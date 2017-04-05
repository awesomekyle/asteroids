#include "config.h"
#include <stdio.h>
#include <assert.h>
#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
    #define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
    #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

//////
// GLFW callbacks
//////
static void _GlfwErrorCallback(int error, char const* description)
{
    fprintf(stderr, "GLW Error: %d - %s\n", error, description);
}
static void _GlfwKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    UNREFERENCED(scancode);
    UNREFERENCED(action);
    UNREFERENCED(mods);
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    // Initialize GLFW
    glfwSetErrorCallback(_GlfwErrorCallback);
    if (glfwInit() != GLFW_TRUE) {
        fprintf(stderr, "GLFW Initialization Failed");
        return 1;
    }
    fprintf(stdout, "GLFW initialization succeeded: %s\n", glfwGetVersionString());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* const window = glfwCreateWindow(800, 600, "Asteroids", NULL, NULL);
    glfwSetKeyCallback(window, _GlfwKeyboardCallback);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
