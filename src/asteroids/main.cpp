#include <iostream>

int main(int argc, char const* const argv[])
{
    for (int ii = 0; ii < argc; ++ii) {
        std::cout << argv[ii] << std::endl;
    }
    return 0;
}
