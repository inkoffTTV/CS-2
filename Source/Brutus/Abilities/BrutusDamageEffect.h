#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BrutusDamageEffect.generated.h"

class UAbilitySystemComponent;

// Instant damage: adds the SetByCaller value named DamageName (pass it negative) to Health.
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
