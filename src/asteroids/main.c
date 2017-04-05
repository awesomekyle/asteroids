#include "config.h"
#include <stdio.h>
#include <stdbool.h>
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
#include "gfx/gfx.h"

//////
// Variables
//////
static Gfx* gGfx = NULL;

//////
// Functions
//////
static bool _InitializeApp(void)
{
    gGfx = gfxCreate();
    if(gGfx == NULL) {
        assert(gGfx != NULL && "Could not create Gfx device");
        return false;
    }
    return true;
}

//////
// GLFW callbacks
//////
static void _GlfwErrorCallback(int error, char const* description)
{
    fprintf(stderr, "GLW Error: %d - %s\n", error, description);
}
static void _GlfwKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    UNREFERENCED(scancode); UNREFERENCED(action); UNREFERENCED(mods);
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}

int main(int argc, char* argv[])
{
    UNREFERENCED(argc); UNREFERENCED(argv);

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

    // Initialize app
    bool const result = _InitializeApp();
    if(result != true) {
        fprintf(stderr, "Could not initialize app\n");
        return 2;
    }

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
