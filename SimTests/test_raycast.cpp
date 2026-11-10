#include "third_party/doctest.h"
#include "Raycast.h"

using namespace bsim;

static const std::vector<Capsule> Target{
    Capsule{{0, 0, 0}, {0, 0, 100}, 30.f, HitZone::Body},
    Capsule{{0, 0, 150}, {0, 0, 150}, 15.f, HitZone::Head},
};

TEST_CASE("Ray hits the body capsule side")
{
    const auto Hit = RaycastCapsules(Ray{{-500, 0, 50}, {1, 0, 0}}, Target, 10000.f);
    REQUIRE(Hit.has_value());
    CHECK(Hit->Zone == HitZone::Body);
    CHECK(Hit->ShapeIndex == 0);
    CHECK(Hit->Distance == doctest::Approx(470.f));
}

TEST_CASE("Ray hits the head sphere")
{
    const auto Hit = RaycastCapsules(Ray{{-500, 0, 150}, {1, 0, 0}}, Target, 10000.f);
    REQUIRE(Hit.has_value());
    CHECK(Hit->Zone == HitZone::Head);
    CHECK(Hit->Distance == doctest::Approx(485.f));
}

TEST_CASE("Ray hits the rounded capsule end cap")
{
    // Body top cap: sphere of radius 30 at z=100. Ray at z=120 meets it at x = -sqrt(30^2-20^2).
    const auto Hit = RaycastCapsules(Ray{{-500, 0, 120}, {1, 0, 0}}, Target, 10000.f);
    REQUIRE(Hit.has_value());
    CHECK(Hit->Zone == HitZone::Body);
    CHECK(Hit->Distance == doctest::Approx(500.f - 22.3607f).epsilon(1e-3));
}

TEST_CASE("Ray above the target misses")
{
    CHECK_FALSE(RaycastCapsules(Ray{{-500, 0, 200}, {1, 0, 0}}, Target, 10000.f).has_value());
}

TEST_CASE("MaxDistance limits hits")
{
    CHECK_FALSE(RaycastCapsules(Ray{{-500, 0, 50}, {1, 0, 0}}, Target, 400.f).has_value());
}

TEST_CASE("Target behind the ray origin is not hit")
{
    CHECK_FALSE(RaycastCapsules(Ray{{-500, 0, 50}, {-1, 0, 0}}, Target, 10000.f).has_value());
}

TEST_CASE("Ray starting inside a capsule hits at distance zero")
{
    const auto Hit = RaycastCapsules(Ray{{0, 0, 50}, {1, 0, 0}}, Target, 10000.f);
    REQUIRE(Hit.has_value());
    CHECK(Hit->Distance == doctest::Approx(0.f));
}

TEST_CASE("Rewind time is clamped to [now - MaxRewind, now]")
{
    CHECK(ClampRewindTime(10.0, 9.9) == doctest::Approx(9.9));
    CHECK(ClampRewindTime(10.0, 9.5) == doctest::Approx(9.75));
    CHECK(ClampRewindTime(10.0, 10.2) == doctest::Approx(10.0));
}
