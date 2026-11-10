#include "third_party/doctest.h"
#include "Overlap.h"

using namespace bsim;

namespace
{
const std::vector<Capsule> Target{
    Capsule{{0, 0, 0}, {0, 0, 100}, 30.f, HitZone::Body},
    Capsule{{0, 0, 150}, {0, 0, 150}, 15.f, HitZone::Head},
};
} // namespace

TEST_CASE("Sphere touching the capsule side overlaps")
{
    CHECK(SphereOverlapsCapsules({100, 0, 50}, 80.f, Target)); // 100 - 30 = 70 < 80
}

TEST_CASE("Sphere clear of the capsule does not overlap")
{
    CHECK_FALSE(SphereOverlapsCapsules({200, 0, 50}, 80.f, Target));
}

TEST_CASE("Sphere near a capsule end cap overlaps by endpoint distance")
{
    CHECK(SphereOverlapsCapsules({0, 0, -20}, 10.f, Target));        // 20 from A, 30 radius
    CHECK_FALSE(SphereOverlapsCapsules({0, 100, -20}, 10.f, Target)); // far
}

TEST_CASE("Sphere can hit only the head sphere")
{
    CHECK(SphereOverlapsCapsules({0, 0, 190}, 30.f, Target)); // 40 from head center, 45 reach
    CHECK_FALSE(SphereOverlapsCapsules({0, 0, 200}, 30.f, Target));
}

TEST_CASE("Empty capsule list never overlaps")
{
    CHECK_FALSE(SphereOverlapsCapsules({0, 0, 0}, 1000.f, {}));
}
