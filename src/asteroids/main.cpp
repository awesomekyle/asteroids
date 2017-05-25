#include <iostream>
#include <gsl\span>

int main(int const argc, char const* const argv[])
{
    for (auto const* const arg : gsl::make_span(argv, argc)) {
        std::cout << arg << std::endl;
    }
    return 0;
}
