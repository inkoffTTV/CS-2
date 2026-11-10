#include "third_party/doctest.h"
#include "Zipline.h"

#include <cmath>

using namespace bsim;

static const ZiplineSegment Seg{{0, 0, 0}, {1000, 0, 0}};

TEST_CASE("ProjectT finds the nearest point parameter, clamped")
{
    CHECK(ProjectT(Seg, {250, 50, 0}) == doctest::Approx(0.25f));
    CHECK(ProjectT(Seg, {-100, 0, 0}) == doctest::Approx(0.f));
    CHECK(ProjectT(Seg, {2000, 0, 0}) == doctest::Approx(1.f));
}

TEST_CASE("PointAt and DistanceTo")
{
    CHECK(PointAt(Seg, 0.5f).X == doctest::Approx(500.f));
    CHECK(DistanceTo(Seg, {250, 50, 0}) == doctest::Approx(50.f));
}

TEST_CASE("AdvanceT moves by speed and clamps at the ends")
{
    CHECK(AdvanceT(Seg, 0.9f, +1, 1800.f, 0.1f) == doctest::Approx(1.f));
    CHECK(AdvanceT(Seg, 0.5f, -1, 1000.f, 0.1f) == doctest::Approx(0.4f));
}

TEST_CASE("ChooseDirection follows facing")
{
    CHECK(ChooseDirection(Seg, {-1, 0, 0}) == -1);
    CHECK(ChooseDirection(Seg, {1, 0.2f, 0}) == 1);
}

TEST_CASE("Degenerate segment does not produce NaN")
{
    const ZiplineSegment Point{{5, 5, 5}, {5, 5, 5}};
    const float T = ProjectT(Point, {100, 0, 0});
    CHECK_FALSE(std::isnan(T));
    CHECK(T == doctest::Approx(0.f));
    CHECK(AdvanceT(Point, 0.f, +1, 1000.f, 0.1f) == doctest::Approx(1.f));
}
