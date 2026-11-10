#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

#include <vector>

namespace bsim
{
// Shotgun pellet directions: Count unit vectors uniformly distributed (by solid angle) inside a cone of
// SpreadDegrees around Forward. Deterministic for a given Seed on every platform (own PRNG), so the client
// can send the seed and the server reproduces the pattern. Spread <= 0 returns Forward unchanged.
BRUTUSSIM_API std::vector<Vec3> PelletDirections(const Vec3& Forward, float SpreadDegrees, int Count, unsigned Seed);
} // namespace bsim
