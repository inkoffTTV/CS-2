#pragma once

#include "BrutusSimApi.h"
#include "HitboxHistory.h"
#include "Vec3.h"

#include <optional>
#include <vector>

namespace bsim
{
struct Ray
{
    Vec3 Origin;
    Vec3 Dir; // normalized
};

struct RayHit
{
    float Distance = 0.f;
    HitZone Zone = HitZone::Body;
    int ShapeIndex = -1;
};

// Nearest hit against the capsules within MaxDistance. A ray starting inside a capsule hits at 0.
BRUTUSSIM_API std::optional<RayHit> RaycastCapsules(const Ray& R, const std::vector<Capsule>& Shapes, float MaxDistance);

// Server time to rewind a shot to: ServerNow minus the shooter's latency, capped at MaxRewind.
// Client times in the future are treated as "now".
BRUTUSSIM_API double ClampRewindTime(double ServerNow, double ClientShotTime, double MaxRewind = 0.25);
} // namespace bsim
