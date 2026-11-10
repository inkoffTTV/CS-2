#pragma once

#include "BrutusSimApi.h"

namespace bsim
{
// Shared stamina pool for dash and air jumps. Charges are fractional while regenerating.
struct StaminaConfig
{
    int MaxCharges = 3;
    float RegenSecondsPerCharge = 2.5f;
};

struct StaminaState
{
    float Charges = 3.f;
};

// Spends Cost charges only if at least Cost are available; otherwise leaves the state untouched.
BRUTUSSIM_API bool TryConsume(StaminaState& State, const StaminaConfig& Config, int Cost = 1);

// Regenerates charges over DeltaSeconds, clamped to MaxCharges.
BRUTUSSIM_API void Regen(StaminaState& State, const StaminaConfig& Config, float DeltaSeconds);
} // namespace bsim
