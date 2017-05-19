#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#if defined(_WIN32)
#   define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#   define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#   define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "gfx/gfx.h"

//////
// Constants
//////
enum {
    kWindowWidth  = 1920,
    kWindowHeight = 1080,
};

//////
// Variables
//////
static Gfx* gGfx = NULL;

//////
// Functions
//////
static void* _PlatformWindow(GLFWwindow* const window)
{
#if defined(_WIN32)
    return glfwGetWin32Window(window);
#elif defined(__APPLE__)
    return glfwGetCocoaWindow(window);
#else
#warning "Not passing native window to Gfx"
    return NULL;
#endif
}
static void* NativeInstance(void)
{
#if defined(_WIN32)
    return GetModuleHandle(NULL);
#elif defined(__APPLE__)
    return NULL; // TODO: Get NSApplication; not yet required
#else
#warning "Not passing native application"
    return NULL;
#endif
}

static bool _InitializeApp(GLFWwindow* const window)
{
    gGfx = gfxCreate(kGfxApiDefault);
    if (gGfx == NULL) {
        assert(gGfx != NULL && "Could not create Gfx device");
        return false;
    }
    bool const result = gfxCreateSwapChain(gGfx,
                                           _PlatformWindow(window),
                                           NativeInstance());
    if (result == false) {
        assert(result == true && "Could not create swap chain for window");
        return false;
    }
    return true;
}
static void _ShutdownApp(void)
{
    gfxDestroy(gGfx);
}

static float _GetWindowScale(GLFWwindow* window)
{
    int windowWidth = 0;
    int framebufferWidth = 0;
    glfwGetWindowSize(window, &windowWidth, NULL);
    glfwGetFramebufferSize(window, &framebufferWidth, NULL);
    return framebufferWidth / (float)windowWidth;
}
static void _SetFramebufferSize(GLFWwindow* window, int width, int height)
{
    float const scale = _GetWindowScale(window);
    glfwSetWindowSize(window, (int)(width / scale), (int)(height / scale));
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
    UNREFERENCED(scancode);
    UNREFERENCED(action);
    UNREFERENCED(mods);
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}
static void _GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    UNREFERENCED(window);
    gfxResize(gGfx, width, height);
}

int main(int argc, char* argv[])
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);

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
    glfwSetFramebufferSizeCallback(window, _GlfwFramebufferSizeCallback);


    // Initialize app
    bool const result = _InitializeApp(window);
    if (result != true) {
        fprintf(stderr, "Could not initialize app\n");
        return 2;
    }
    _SetFramebufferSize(window, kWindowWidth, kWindowHeight); // make the resize happen

    // Create graphics objects
    char const* vsSource = NULL;
    char const* psSource = NULL;
    if (gfxGetApi(gGfx) == kGfxApiD3D12) {
        vsSource =
            "float4 main(uint vertexId : SV_VertexID) : SV_POSITION {"\
            "    float2 pos[3] = { float2(-0.7, 0.7), float2(0.7, 0.7), float2(0.0, -0.7) };"\
            "    return float4(pos[vertexId], 0.0f, 1.0f);"\
            "}";
        psSource =
            "float4 main() : SV_TARGET {"\
            "    return float4(0.3f, 0.5f, 0.7f, 1.0f);"
            "}";

    }
    GfxRenderStateDesc const desc = {
        .vertexShader = {
            .source = vsSource,
            .entrypoint = "main",
        },
        .pixelShader = {
            .source = psSource,
            .entrypoint = "main",
        },
        .layout = NULL,
        .depthFormat = kUnknown,
        .culling = kNone,
        .depthWrite = false,
        .name = "Simple State",
    };
    GfxRenderState* const renderState = gfxCreateRenderState(gGfx, &desc);

    int frameCount = 0;
    float elapsedTime = 0.0f;
    uint64_t prevTime = glfwGetTimerValue();

    float color = 0.0f;
    // main loop
    while (!glfwWindowShouldClose(window)) {
        uint64_t const currTime = glfwGetTimerValue();
        float const deltaTime = (currTime - prevTime) / (float)(glfwGetTimerFrequency());
        prevTime = currTime;
        if (elapsedTime > 0.5f) {
            fprintf(stdout, "FPS: %d\n", frameCount * 2);
            frameCount = 0;
            elapsedTime -= 0.5f;
        }
        frameCount++;
        elapsedTime += deltaTime;
        color += deltaTime;
        glfwPollEvents();

        float const clearColor[] = {
            sinf(color) * 0.5f + 0.5f,
            cosf(color * 1.1f) * 0.5f + 0.5f,
            sinf(color * 0.7f)* cosf(0.9f) * 0.5f + 0.5f,
            1.0f
        };
        GfxCmdBuffer* const buffer = gfxGetCommandBuffer(gGfx);
        gfxCmdBeginRenderPass(buffer, gfxGetBackBuffer(gGfx), kGfxRenderPassActionClear, clearColor);
        gfxCmdEndRenderPass(buffer);
        gfxExecuteCommandBuffer(buffer);

        gfxPresent(gGfx);
    }

    gfxDestroyRenderState(gGfx, renderState);

    _ShutdownApp();
    glfwTerminate();

    return 0;
}
