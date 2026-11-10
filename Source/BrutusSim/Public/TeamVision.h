#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace bsim
{
struct VisionActor
{
    int Id = 0;
    int Team = 0;
    Vec3 Pos;
    float SightRadius = 0.f;
    bool bObserver = false; // heroes and structures give vision
    bool bHidden = false;   // invisible (Nemo): revealed only up close
};

struct VisionConfig
{
    double GraceSeconds = 0.2; // the UE net driver adds ~1 s (RelevantTimeout) on top
    float HiddenRevealRadius = 600.f;
};

using LineOfSightFn = std::function<bool(const VisionActor& Observer, const VisionActor& Target)>;

// Server-side team vision: which enemies each team currently sees.
class BRUTUSSIM_API TeamVision
{
public:
    explicit TeamVision(VisionConfig InConfig = {});

    void Update(double Now, const std::vector<VisionActor>& Actors, const LineOfSightFn& HasLineOfSight);

    bool IsVisibleToTeam(int TargetId, int Team) const;

private:
    struct Known
    {
        int Team = 0;
        bool bHidden = false;
    };

    static long long Key(int TargetId, int Team);

    VisionConfig Config;
    double LastUpdate = 0.0;
    std::unordered_map<int, Known> Actors;
    std::unordered_map<long long, double> LastSeen;
};
} // namespace bsim
