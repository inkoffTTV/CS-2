#include "third_party/doctest.h"
#include "Ballistics.h"

#include <cmath>

using namespace bsim;

namespace
{
Vec3 Position(const Vec3& From, const Vec3& V, float G, float T)
{
    return From + V * T + Vec3{0, 0, 0.5f * G * T * T};
}
} // namespace

TEST_CASE("Launch velocity reaches the target after the flight time")
{
    const Vec3 From{100, -50, 20};
    const Vec3 To{600, 300, 150};
    const float G = -980.f;
    const float T = 0.45f;
    const Vec3 V = LaunchVelocityToReach(From, To, G, T);
    const Vec3 P = Position(From, V, G, T);
    CHECK(std::fabs(P.X - To.X) < 1.f);
    CHECK(std::fabs(P.Y - To.Y) < 1.f);
    CHECK(std::fabs(P.Z - To.Z) < 1.f);
}

TEST_CASE("Zero gravity is a straight line")
{
    const Vec3 V = LaunchVelocityToReach({0, 0, 0}, {100, 0, 0}, 0.f, 0.5f);
    CHECK(V.X == doctest::Approx(200.f));
    CHECK(V.Y == doctest::Approx(0.f));
    CHECK(V.Z == doctest::Approx(0.f));
}

TEST_CASE("Non-positive flight time returns the zero vector")
{
    const Vec3 V = LaunchVelocityToReach({0, 0, 0}, {100, 0, 0}, -980.f, 0.f);
    CHECK(V.X == 0.f);
    CHECK(V.Y == 0.f);
    CHECK(V.Z == 0.f);
}
