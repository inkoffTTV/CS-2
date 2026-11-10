#include "third_party/doctest.h"
#include "AreaDamage.h"

using namespace bsim;

TEST_CASE("Radial falloff is 1 at the center")
{
    CHECK(RadialFalloff(0.f, 350.f, 0.3f) == doctest::Approx(1.f));
}

TEST_CASE("Radial falloff is linear to MinFraction at the edge")
{
    CHECK(RadialFalloff(175.f, 350.f, 0.3f) == doctest::Approx(0.65f));
    CHECK(RadialFalloff(350.f, 350.f, 0.3f) == doctest::Approx(0.3f));
}

TEST_CASE("Radial falloff is zero beyond the radius")
{
    CHECK(RadialFalloff(350.01f, 350.f, 0.3f) == 0.f);
    CHECK(RadialFalloff(1000.f, 350.f, 0.3f) == 0.f);
}

TEST_CASE("Radial falloff handles degenerate input")
{
    CHECK(RadialFalloff(-5.f, 350.f, 0.3f) == doctest::Approx(1.f));
    CHECK(RadialFalloff(0.f, 0.f, 0.3f) == 0.f);
}
