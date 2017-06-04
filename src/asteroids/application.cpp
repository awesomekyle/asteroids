#include "application.h"

Application::Application(GLFWWindow* window, ak::Graphics* graphics)
    : _window(window)
    , _graphics(graphics)
{
}

Application::~Application()
{
}
