#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

namespace bsim
{
// Initial velocity V such that a point launched from From under constant acceleration (0, 0, Gravity)
// is at To after FlightTime: To = From + V*t + 0.5*g*t^2. Gravity is signed (UE: negative Z).
// FlightTime <= 0 returns the zero vector.
BRUTUSSIM_API Vec3 LaunchVelocityToReach(const Vec3& From, const Vec3& To, float Gravity, float FlightTime);
} // namespace bsim
