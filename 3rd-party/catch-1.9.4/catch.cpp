#define CATCH_CONFIG_MAIN
#define DO_NOT_USE_WMAIN
#if defined(_WIN32)
    #define CATCH_CONFIG_WINDOWS_CRTDBG
#endif // _WIN32
#include "catch.hpp"
