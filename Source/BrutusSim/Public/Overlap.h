#pragma once

#include "BrutusSimApi.h"
#include "HitboxHistory.h"
#include "Vec3.h"

#include <vector>

namespace bsim
{
// True if the sphere (Center, Radius) touches any of the capsules.
BRUTUSSIM_API bool SphereOverlapsCapsules(const Vec3& Center, float Radius, const std::vector<Capsule>& Shapes);
} // namespace bsim
