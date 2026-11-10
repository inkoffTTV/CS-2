#include "Stamina.h"

#include <algorithm>

namespace bsim
{
bool TryConsume(StaminaState& State, const StaminaConfig& Config, int Cost)
{
    (void)Config;
    if (Cost <= 0 || State.Charges < static_cast<float>(Cost))
    {
        return false;
    }
    State.Charges = std::max(0.f, State.Charges - static_cast<float>(Cost));
    return true;
}

void Regen(StaminaState& State, const StaminaConfig& Config, float DeltaSeconds)
{
    if (DeltaSeconds <= 0.f || Config.RegenSecondsPerCharge <= 0.f)
    {
        return;
    }
    const float Max = static_cast<float>(Config.MaxCharges);
    State.Charges = std::min(Max, State.Charges + DeltaSeconds / Config.RegenSecondsPerCharge);
}
} // namespace bsim
