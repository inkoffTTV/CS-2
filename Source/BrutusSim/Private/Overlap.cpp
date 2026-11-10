#include "Overlap.h"

namespace bsim
{
namespace
{
float DistanceSqToSegment(const Vec3& P, const Vec3& A, const Vec3& B)
{
    const Vec3 AB = B - A;
    const float LenSq = LengthSq(AB);
    float T = LenSq > 1e-12f ? Dot(P - A, AB) / LenSq : 0.f;
    T = T < 0.f ? 0.f : (T > 1.f ? 1.f : T);
    return LengthSq(P - (A + AB * T));
}
} // namespace

bool SphereOverlapsCapsules(const Vec3& Center, float Radius, const std::vector<Capsule>& Shapes)
{
    for (const Capsule& C : Shapes)
    {
        const float Reach = Radius + C.Radius;
        if (DistanceSqToSegment(Center, C.A, C.B) <= Reach * Reach)
        {
            return true;
        }
    }
    return false;
}
} // namespace bsim
