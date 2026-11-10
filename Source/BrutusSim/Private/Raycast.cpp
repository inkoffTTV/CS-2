#include "Raycast.h"

#include <algorithm>
#include <cmath>

namespace bsim
{
namespace
{
constexpr float NoHit = -1.f;

float DistanceToSegmentSq(const Vec3& P, const Vec3& A, const Vec3& B)
{
    const Vec3 AB = B - A;
    const float LenSq = LengthSq(AB);
    const float T = LenSq > 1e-6f ? std::clamp(Dot(P - A, AB) / LenSq, 0.f, 1.f) : 0.f;
    return LengthSq(P - (A + AB * T));
}

float RaySphere(const Ray& R, const Vec3& Center, float Radius)
{
    const Vec3 OC = R.Origin - Center;
    const float B = Dot(OC, R.Dir);
    const float C = LengthSq(OC) - Radius * Radius;
    const float H = B * B - C;
    if (H < 0.f)
    {
        return NoHit;
    }
    const float T = -B - std::sqrt(H);
    return T >= 0.f ? T : NoHit;
}

// Side of the finite cylinder between A and B (caps are handled by the end spheres).
float RayCylinderSide(const Ray& R, const Vec3& A, const Vec3& B, float Radius)
{
    const Vec3 BA = B - A;
    const Vec3 OA = R.Origin - A;
    const float BaBa = Dot(BA, BA);
    const float BaRd = Dot(BA, R.Dir);
    const float BaOa = Dot(BA, OA);
    const float QA = BaBa - BaRd * BaRd;
    if (BaBa < 1e-6f || QA < 1e-6f)
    {
        return NoHit; // sphere or ray parallel to the axis
    }
    const float QB = BaBa * Dot(R.Dir, OA) - BaOa * BaRd;
    const float QC = BaBa * Dot(OA, OA) - BaOa * BaOa - Radius * Radius * BaBa;
    const float H = QB * QB - QA * QC;
    if (H < 0.f)
    {
        return NoHit;
    }
    const float T = (-QB - std::sqrt(H)) / QA;
    const float Y = BaOa + T * BaRd;
    return (T >= 0.f && Y > 0.f && Y < BaBa) ? T : NoHit;
}

float RayCapsule(const Ray& R, const Capsule& C)
{
    if (DistanceToSegmentSq(R.Origin, C.A, C.B) <= C.Radius * C.Radius)
    {
        return 0.f;
    }
    float Best = NoHit;
    for (const float T : {RayCylinderSide(R, C.A, C.B, C.Radius), RaySphere(R, C.A, C.Radius), RaySphere(R, C.B, C.Radius)})
    {
        if (T >= 0.f && (Best < 0.f || T < Best))
        {
            Best = T;
        }
    }
    return Best;
}
} // namespace

std::optional<RayHit> RaycastCapsules(const Ray& R, const std::vector<Capsule>& Shapes, float MaxDistance)
{
    std::optional<RayHit> Result;
    for (std::size_t I = 0; I < Shapes.size(); ++I)
    {
        const float T = RayCapsule(R, Shapes[I]);
        if (T >= 0.f && T <= MaxDistance && (!Result || T < Result->Distance))
        {
            Result = RayHit{T, Shapes[I].Zone, static_cast<int>(I)};
        }
    }
    return Result;
}

double ClampRewindTime(double ServerNow, double ClientShotTime, double MaxRewind)
{
    if (!std::isfinite(ClientShotTime))
    {
        return ServerNow;
    }
    return ServerNow - std::clamp(ServerNow - ClientShotTime, 0.0, MaxRewind);
}
} // namespace bsim
