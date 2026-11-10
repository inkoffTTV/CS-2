#include "Zipline.h"

#include <algorithm>

namespace bsim
{
float ProjectT(const ZiplineSegment& Seg, const Vec3& Pos)
{
    const Vec3 AB = Seg.B - Seg.A;
    const float LenSq = LengthSq(AB);
    if (LenSq < 1e-6f)
    {
        return 0.f;
    }
    return std::clamp(Dot(Pos - Seg.A, AB) / LenSq, 0.f, 1.f);
}

Vec3 PointAt(const ZiplineSegment& Seg, float T)
{
    return Lerp(Seg.A, Seg.B, T);
}

float DistanceTo(const ZiplineSegment& Seg, const Vec3& Pos)
{
    return Length(Pos - PointAt(Seg, ProjectT(Seg, Pos)));
}

float AdvanceT(const ZiplineSegment& Seg, float T, int Direction, float Speed, float Dt)
{
    const float Len = Length(Seg.B - Seg.A);
    const float Sign = Direction >= 0 ? 1.f : -1.f;
    if (Len < 1e-3f)
    {
        return Sign > 0.f ? 1.f : 0.f;
    }
    return std::clamp(T + Sign * Speed * Dt / Len, 0.f, 1.f);
}

int ChooseDirection(const ZiplineSegment& Seg, const Vec3& Facing)
{
    return Dot(Flatten(Facing), Flatten(Seg.B - Seg.A)) >= 0.f ? 1 : -1;
}
} // namespace bsim
