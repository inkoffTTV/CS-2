#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BrutusMalleusEffects.generated.h"

// Ground slam slow: MoveSpeedMultiplier x0.7 for 2 s. Several slows stack multiplicatively-additive (GAS rules).
UCLASS()
class BRUTUS_API UBrutusMalleusSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusMalleusSlowEffect();

	// Starting values for tuning.
	static constexpr float SpeedMultiplier = 0.7f;
	static constexpr float DurationSeconds = 2.f;
};

// Rage: DamageTakenMultiplier x0.7 for 10 s (the Rage ability also removes it when it ends).
UCLASS()
class BRUTUS_API UBrutusMalleusRageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusMalleusRageEffect();

	static constexpr float DamageTakenMultiplier = 0.7f;
	static constexpr float DurationSeconds = 10.f;
};

// Instant heal of the SetByCaller amount HealName. Health is clamped to MaxHealth by the attribute set.
UCLASS()
class BRUTUS_API UBrutusMalleusHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusMalleusHealEffect();

	static const FName HealName;
};
