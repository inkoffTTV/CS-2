#include "TeamVision.h"

#include <iterator>

namespace bsim
{
TeamVision::TeamVision(VisionConfig InConfig)
    : Config(InConfig)
{
}

long long TeamVision::Key(int TargetId, int Team)
{
    return (static_cast<long long>(TargetId) << 32) ^ static_cast<unsigned int>(Team);
}

void TeamVision::Update(double Now, const std::vector<VisionActor>& InActors, const LineOfSightFn& HasLineOfSight)
{
    LastUpdate = Now;
    Actors.clear();
    for (const VisionActor& A : InActors)
    {
        Actors[A.Id] = Known{A.Team, A.bHidden};
    }

    // Forget ids that are gone so a re-appearing id gets no inherited grace.
    for (auto It = LastSeen.begin(); It != LastSeen.end();)
    {
        const int TargetId = static_cast<int>(It->first >> 32);
        It = Actors.count(TargetId) ? std::next(It) : LastSeen.erase(It);
    }

    for (const VisionActor& Target : InActors)
    {
        for (const VisionActor& Obs : InActors)
        {
            if (!Obs.bObserver || Obs.Team == Target.Team)
            {
                continue;
            }
            const float Range = Target.bHidden ? Config.HiddenRevealRadius : Obs.SightRadius;
            if (LengthSq(Target.Pos - Obs.Pos) > Range * Range || !HasLineOfSight(Obs, Target))
            {
                continue;
            }
            LastSeen[Key(Target.Id, Obs.Team)] = Now;
        }
    }
}

bool TeamVision::IsVisibleToTeam(int TargetId, int Team) const
{
    const auto It = Actors.find(TargetId);
    if (It == Actors.end())
    {
        return false;
    }
    if (It->second.Team == Team)
    {
        return true;
    }
    const auto Seen = LastSeen.find(Key(TargetId, Team));
    if (Seen == LastSeen.end())
    {
        return false;
    }
    const double Grace = It->second.bHidden ? 0.0 : Config.GraceSeconds;
    return LastUpdate - Seen->second <= Grace;
}
} // namespace bsim
