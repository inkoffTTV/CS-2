#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

namespace bsim
{
// A straight zipline cable from A to B in world space. T = 0 at A, T = 1 at B.
struct ZiplineSegment
{
    Vec3 A;
    Vec3 B;
};

struct ZiplineConfig
{
    float Speed = 1800.f;        // cm/s along the cable
    float AttachRadius = 200.f;  // max distance from cable to grab it
    float HangOffsetZ = -120.f;  // character location relative to the cable
};

BRUTUSSIM_API float ProjectT(const ZiplineSegment& Seg, const Vec3& Pos);
BRUTUSSIM_API Vec3 PointAt(const ZiplineSegment& Seg, float T);
BRUTUSSIM_API float DistanceTo(const ZiplineSegment& Seg, const Vec3& Pos);

// Moves T along the cable by Speed * Dt in Direction (+1 toward B, -1 toward A), clamped to [0, 1].
// A zero-length cable is finished immediately (returns the end in Direction).
BRUTUSSIM_API float AdvanceT(const ZiplineSegment& Seg, float T, int Direction, float Speed, float Dt);

// +1 when the horizontal facing points toward B, otherwise -1.
BRUTUSSIM_API int ChooseDirection(const ZiplineSegment& Seg, const Vec3& Facing);
} // namespace bsim
