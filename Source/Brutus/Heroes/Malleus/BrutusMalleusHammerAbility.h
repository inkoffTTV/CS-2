#pragma once

#include "CoreMinimal.h"
#include "Heroes/Malleus/BrutusMalleusAbility.h"
#include "BrutusMalleusHammerAbility.generated.h"

// RMB: hammer swing. Lag-compensated melee: a sphere in front of Malleus's eyes against enemies'
// hitboxes rewound to the moment the client swung; every enemy touched takes Damage once.
UCLASS()
class BRUTUS_API UBrutusMalleusHammerAbility : public UBrutusMalleusAbility
{
	GENERATED_BODY()

public:
	UBrutusMalleusHammerAbility();

	// Starting values for tuning.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Damage = 75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float SphereRadius = 120.f;

	// Horizontal distance from the eyes to the sphere centre.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float ReachForward = 150.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnAimReady(const FBrutusMalleusAimTargetData& Aim, bool bAuthority, bool bLocal) override;
};
