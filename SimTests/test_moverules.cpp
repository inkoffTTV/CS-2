#include "third_party/doctest.h"
#include "MoveRules.h"

#include <cmath>

using namespace bsim;

static void CheckVec(const Vec3& V, float X, float Y, float Z)
{
    CHECK(V.X == doctest::Approx(X).epsilon(1e-3));
    CHECK(V.Y == doctest::Approx(Y).epsilon(1e-3));
    CHECK(V.Z == doctest::Approx(Z).epsilon(1e-3));
}

TEST_CASE("Dash goes horizontally along the wish direction")
{
    MoveConfig Cfg;
    CheckVec(ComputeDashVelocity({1, 0, 0.5f}, {0, 1, 0}, Cfg), 1500, 0, 0);
}

TEST_CASE("Dash with no input uses facing")
{
    MoveConfig Cfg;
    CheckVec(ComputeDashVelocity({0, 0, 0}, {0, 1, 0}, Cfg), 0, 1500, 0);
}

TEST_CASE("Air jumps are limited and reset on landing")
{
    MoveConfig Cfg;
    AirJumpState S;
    CHECK(TryAirJump(S, Cfg));
    CHECK_FALSE(TryAirJump(S, Cfg));
    ResetAirJumps(S);
    CHECK(TryAirJump(S, Cfg));
}

TEST_CASE("Slide needs speed and ground")
{
    MoveConfig Cfg;
    CHECK_FALSE(CanStartSlide(499.f, true, Cfg));
    CHECK_FALSE(CanStartSlide(600.f, false, Cfg));
    CHECK(CanStartSlide(600.f, true, Cfg));
}

TEST_CASE("Slide start adds boost and clamps to max")
{
    MoveConfig Cfg;
    CheckVec(StartSlideVelocity({1000, 0, 0}, Cfg), 1250, 0, 0);
    CheckVec(StartSlideVelocity({1700, 0, 0}, Cfg), 1800, 0, 0);
}

TEST_CASE("Slide decelerates on flat ground")
{
    MoveConfig Cfg;
    CheckVec(StepSlide({1000, 0, 0}, {0, 0, 1}, 0.5f, Cfg), 800, 0, 0);
}

TEST_CASE("Slide accelerates downhill and stays on the slope plane")
{
    MoveConfig Cfg;
    const Vec3 N{0.5f, 0.f, std::sqrt(3.f) / 2.f}; // 30 degrees, downhill toward +X
    const Vec3 V = StepSlide({1000, 0, 0}, N, 1.0f, Cfg);
    CHECK(Length(V) == doctest::Approx(1090.f).epsilon(1e-3));
    CHECK(std::fabs(Dot(V, N)) < 1e-2f);
}

TEST_CASE("Slide friction never reverses velocity")
{
    MoveConfig Cfg;
    CheckVec(StepSlide({100, 0, 0}, {0, 0, 1}, 1.0f, Cfg), 0, 0, 0);
}

TEST_CASE("Slide exits below exit speed")
{
    MoveConfig Cfg;
    CHECK(ShouldExitSlide({299, 0, 0}, Cfg));
    CHECK_FALSE(ShouldExitSlide({301, 0, 0}, Cfg));
}
