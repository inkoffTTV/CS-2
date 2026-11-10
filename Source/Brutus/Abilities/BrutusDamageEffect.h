#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BrutusDamageEffect.generated.h"

class UAbilitySystemComponent;

// Instant damage through UBrutusDamageExecution: the SetByCaller value DamageName (positive)
// scaled by the target's DamageTakenMultiplier is subtracted from Health.
UCLASS()
class BRUTUS_API UBrutusDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusDamageEffect();

	static const FName DamageName;

	// Applies Amount (positive) damage from Source to Target. Server only.
	static void Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, float Amount);
};
