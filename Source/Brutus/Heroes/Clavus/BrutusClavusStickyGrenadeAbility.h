#pragma once

#include "CoreMinimal.h"
#include "Abilities/BrutusGameplayAbility.h"
#include "BrutusClavusStickyGrenadeAbility.generated.h"

class ABrutusClavusStickyGrenade;

// Clavus Q: throws a sticky grenade along the aim. The server spawns the (replicated) grenade;
// the owning client only predicts the cooldown, so one press throws exactly one grenade.
UCLASS()
class BRUTUS_API UBrutusClavusStickyGrenadeAbility : public UBrutusGameplayAbility
{
	GENERATED_BODY()

public:
	UBrutusClavusStickyGrenadeAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	TSubclassOf<ABrutusClavusStickyGrenade> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float ThrowSpeed = 1800.f;

	// Extra pitch above the crosshair so the throw arcs up before gravity brings it down.
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float ThrowPitchUpDegrees = 4.f;

	// Spawn point distance in front of the eyes (outside the capsule).
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float SpawnForwardOffset = 60.f;

private:
	void ServerThrow(const FGameplayAbilityActorInfo* ActorInfo) const;
};
