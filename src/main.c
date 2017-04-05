#include <stdio.h>

int main(int argc, char* argv[])
{
    for(int ii = 0; ii < argc; ++ii) {
        printf("%s\n", argv[ii]);
    }
    return 0;
}
