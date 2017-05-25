#include <iostream>

int main(int argc, char* argv[])
{
    for (int ii = 0; ii < argc; ++ii) {
        std::cout << argv[ii];
    }
    return 0;
}
