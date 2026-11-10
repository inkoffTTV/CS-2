#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

namespace bsim
{
enum class HitZone : unsigned char
{
    Body,
    Head
};

// A capsule between A and B with Radius. A sphere is a capsule with A == B.
struct Capsule
{
    Vec3 A;
    Vec3 B;
    float Radius = 0.f;
    HitZone Zone = HitZone::Body;
};

// Time-stamped hitbox poses of one character, used to rewind shots (lag compensation)
// and, later, Rotor's rewind ability.
class BRUTUSSIM_API HitboxHistory
{
public:
    explicit HitboxHistory(double MaxAgeSeconds = 3.0);

    // Appends a snapshot. Times must increase; older or equal times are ignored.
    // bTeleported marks a discontinuity (respawn, teleport): no interpolation across it.
    void Record(double Time, std::vector<Capsule> Shapes, bool bTeleported = false);

    // Pose at Time. Before the oldest snapshot: nullopt. After the newest: newest.
    std::optional<std::vector<Capsule>> SampleAt(double Time) const;

    // Drops snapshots older than Now - MaxAgeSeconds.
    void Prune(double Now);

    std::size_t Num() const { return Snapshots.size(); }

private:
    struct Snapshot
    {
        double Time = 0.0;
        std::vector<Capsule> Shapes;
        bool bTeleported = false;
    };

    double MaxAge;
    std::deque<Snapshot> Snapshots;
};
} // namespace bsim
