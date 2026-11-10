#include "HitboxHistory.h"

#include <algorithm>
#include <utility>

namespace bsim
{
HitboxHistory::HitboxHistory(double MaxAgeSeconds)
    : MaxAge(MaxAgeSeconds)
{
}

void HitboxHistory::Record(double Time, std::vector<Capsule> Shapes, bool bTeleported)
{
    if (!Snapshots.empty() && Time <= Snapshots.back().Time)
    {
        return;
    }
    Snapshots.push_back(Snapshot{Time, std::move(Shapes), bTeleported});
}

std::optional<std::vector<Capsule>> HitboxHistory::SampleAt(double Time) const
{
    if (Snapshots.empty() || Time < Snapshots.front().Time)
    {
        return std::nullopt;
    }
    if (Time >= Snapshots.back().Time)
    {
        return Snapshots.back().Shapes;
    }

    // First snapshot strictly after Time; the one before it is at or before Time.
    const auto Next = std::upper_bound(Snapshots.begin(), Snapshots.end(), Time,
        [](double T, const Snapshot& S) { return T < S.Time; });
    const Snapshot& S1 = *Next;
    const Snapshot& S0 = *(Next - 1);

    if (S1.bTeleported || S0.Shapes.size() != S1.Shapes.size())
    {
        return S0.Shapes;
    }

    const float Alpha = static_cast<float>((Time - S0.Time) / (S1.Time - S0.Time));
    std::vector<Capsule> Out = S0.Shapes;
    for (std::size_t I = 0; I < Out.size(); ++I)
    {
        Out[I].A = Lerp(S0.Shapes[I].A, S1.Shapes[I].A, Alpha);
        Out[I].B = Lerp(S0.Shapes[I].B, S1.Shapes[I].B, Alpha);
        Out[I].Radius = S0.Shapes[I].Radius + (S1.Shapes[I].Radius - S0.Shapes[I].Radius) * Alpha;
    }
    return Out;
}

void HitboxHistory::Prune(double Now)
{
    const double Cutoff = Now - MaxAge;
    while (!Snapshots.empty() && Snapshots.front().Time < Cutoff)
    {
        Snapshots.pop_front();
    }
}
} // namespace bsim
