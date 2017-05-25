#include "catch.hpp"

#include "graphics/graphics.h"

namespace {

TEST_CASE("graphics lifetime")
{
    SECTION("graphics device creation") {
        WHEN("a graphics device is created") {
            auto graphicsDevice = ak::create_graphics();
            THEN("the device is successfully created") {
                REQUIRE(graphicsDevice);
            }
        }
    }
}

} // anonymous

