#include "catch.hpp"
extern "C" {
#include "gfx/gfx.h"
}

namespace {

TEST_CASE("Gfx lifetime")
{
    SECTION("Gfx can be created and destroyed")
    {
        Gfx* const G = gfxCreate();
        REQUIRE(G);
        gfxDestroy(G);
    }
}

} // anonymous
