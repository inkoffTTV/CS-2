#pragma once

#include "BrutusSimApi.h"

namespace bsim
{
// Damage multiplier for a radial blast: 1 at the center, linear down to MinFraction at Radius, 0 beyond.
BRUTUSSIM_API float RadialFalloff(float Distance, float Radius, float MinFraction);
} // namespace bsim
