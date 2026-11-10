#include "AreaDamage.h"

namespace bsim
{
float RadialFalloff(float Distance, float Radius, float MinFraction)
{
    if (Radius <= 0.f || Distance > Radius)
    {
        return 0.f;
    }
    if (Distance <= 0.f)
    {
        return 1.f;
    }
    return 1.f + (MinFraction - 1.f) * (Distance / Radius);
}
} // namespace bsim
