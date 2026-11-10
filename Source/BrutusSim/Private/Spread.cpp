#include "Spread.h"

#include <cmath>
#include <cstdint>

namespace bsim
{
namespace
{
// SplitMix32-style mixer seeds a 32-bit xorshift; both are pure integer math, identical everywhere.
struct Rng
{
    std::uint32_t State;

    explicit Rng(unsigned Seed)
    {
        std::uint32_t Z = static_cast<std::uint32_t>(Seed) + 0x9E3779B9u;
        Z = (Z ^ (Z >> 16)) * 0x85EBCA6Bu;
        Z = (Z ^ (Z >> 13)) * 0xC2B2AE35u;
        Z ^= Z >> 16;
        State = Z != 0 ? Z : 0x1u;
    }

    std::uint32_t Next()
    {
        State ^= State << 13;
        State ^= State >> 17;
        State ^= State << 5;
        return State;
    }

    // Uniform in [0, 1) with 24 bits of precision.
    float NextFloat() { return static_cast<float>(Next() >> 8) * (1.f / 16777216.f); }
};
} // namespace

std::vector<Vec3> PelletDirections(const Vec3& Forward, float SpreadDegrees, int Count, unsigned Seed)
{
    std::vector<Vec3> Result;
    if (Count <= 0)
    {
        return Result;
    }
    Result.reserve(static_cast<std::size_t>(Count));
    if (SpreadDegrees <= 0.f)
    {
        Result.assign(static_cast<std::size_t>(Count), Forward);
        return Result;
    }

    const Vec3 F = Normalized(Forward);
    // Orthonormal basis around F; pick the world axis least aligned with F so F may be straight up.
    const Vec3 Axis = std::fabs(F.Z) < 0.9f ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
    const Vec3 U = Normalized(Cross(Axis, F));
    const Vec3 V = Cross(F, U);

    constexpr float Pi = 3.14159265358979f;
    const float CosMax = std::cos(SpreadDegrees * Pi / 180.f);
    Rng R(Seed);
    for (int I = 0; I < Count; ++I)
    {
        const float CosT = 1.f - R.NextFloat() * (1.f - CosMax);
        const float SinT = std::sqrt(std::fmax(0.f, 1.f - CosT * CosT));
        const float Phi = R.NextFloat() * 2.f * Pi;
        Result.push_back(Normalized(F * CosT + (U * std::cos(Phi) + V * std::sin(Phi)) * SinT));
    }
    return Result;
}
} // namespace bsim
