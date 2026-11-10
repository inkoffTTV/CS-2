#include "third_party/doctest.h"
#include "Spread.h"

#include <cmath>

using namespace bsim;

namespace
{
constexpr float Pi = 3.14159265358979f;
float AngleDeg(const Vec3& A, const Vec3& B)
{
    const float C = std::fmax(-1.f, std::fmin(1.f, Dot(A, B)));
    return std::acos(C) * 180.f / Pi;
}
bool Same(const std::vector<Vec3>& A, const std::vector<Vec3>& B)
{
    if (A.size() != B.size())
    {
        return false;
    }
    for (std::size_t I = 0; I < A.size(); ++I)
    {
        if (A[I].X != B[I].X || A[I].Y != B[I].Y || A[I].Z != B[I].Z)
        {
            return false;
        }
    }
    return true;
}
} // namespace

TEST_CASE("Pellets stay normalized and inside the cone")
{
    for (const Vec3 Fwd : {Vec3{1, 0, 0}, Normalized({1, 2, 3}), Vec3{0, 0, 1}, Vec3{0, 0, -1}})
    {
        const auto Dirs = PelletDirections(Fwd, 5.f, 100, 1234u);
        REQUIRE(Dirs.size() == 100);
        for (const Vec3& D : Dirs)
        {
            CHECK(Length(D) == doctest::Approx(1.f).epsilon(1e-4));
            CHECK(AngleDeg(Fwd, D) <= 5.f + 1e-2f);
        }
    }
}

TEST_CASE("Pellets actually spread out, not all on the axis")
{
    const auto Dirs = PelletDirections({1, 0, 0}, 5.f, 100, 7u);
    float Max = 0.f;
    for (const Vec3& D : Dirs)
    {
        Max = std::fmax(Max, AngleDeg({1, 0, 0}, D));
    }
    CHECK(Max > 3.5f);
}

TEST_CASE("Same seed gives same pellets, different seed differs")
{
    const auto A = PelletDirections({0, 1, 0}, 5.f, 100, 42u);
    const auto B = PelletDirections({0, 1, 0}, 5.f, 100, 42u);
    const auto C = PelletDirections({0, 1, 0}, 5.f, 100, 43u);
    CHECK(Same(A, B));
    CHECK_FALSE(Same(A, C));
}

TEST_CASE("Single pellet with no spread is exactly Forward")
{
    const Vec3 F = Normalized({1, 2, 3});
    const auto Dirs = PelletDirections(F, 0.f, 1, 99u);
    REQUIRE(Dirs.size() == 1);
    CHECK(Dirs[0].X == F.X);
    CHECK(Dirs[0].Y == F.Y);
    CHECK(Dirs[0].Z == F.Z);
}

TEST_CASE("Non-positive count gives no pellets")
{
    CHECK(PelletDirections({1, 0, 0}, 5.f, 0, 1u).empty());
    CHECK(PelletDirections({1, 0, 0}, 5.f, -3, 1u).empty());
}
