#include "Ballistics.h"

namespace bsim
{
Vec3 LaunchVelocityToReach(const Vec3& From, const Vec3& To, float Gravity, float FlightTime)
{
    if (FlightTime <= 0.f)
    {
        return {};
    }
    return (To - From) / FlightTime - Vec3{0.f, 0.f, 0.5f * Gravity * FlightTime};
}
} // namespace bsim
