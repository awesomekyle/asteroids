#pragma once

struct GLFWWindow;
namespace ak {
class Graphics;
}  // namespace ak

class Application
{
   public:
    Application(GLFWWindow* window, ak::Graphics* graphics);
    ~Application();

    GLFWWindow* const _window = nullptr;
    ak::Graphics* const _graphics = nullptr;
};
