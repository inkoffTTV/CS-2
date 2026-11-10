#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BrutusStunEffect.generated.h"

class UAbilitySystemComponent;

// Stun: grants State.Stunned for the SetByCaller duration DurationName.
// While stunned the hero cannot move, jump, dash, fire or activate abilities.
// Always apply it through Apply(): the tag is added to the spec there.
UCLASS()
class BRUTUS_API UBrutusStunEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusStunEffect();

	static const FName DurationName;

	// Stuns Target for Seconds. Server only.
	static void Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, float Seconds);

	// True when the actor's ability system (via IAbilitySystemInterface) has State.Stunned.
	static bool IsActorStunned(const AActor* Actor);
	static bool IsStunned(const UAbilitySystemComponent* ASC);
};
