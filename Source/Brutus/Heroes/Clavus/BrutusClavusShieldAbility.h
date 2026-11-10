#pragma once

#include "CoreMinimal.h"
#include "Abilities/BrutusGameplayAbility.h"
#include "BrutusClavusShieldAbility.generated.h"

// Clavus F: the server stands an ABrutusClavusShield on the floor DeployDistance in front of Clavus,
// facing his yaw. The owning client only predicts the cooldown.
UCLASS()
class BRUTUS_API UBrutusClavusShieldAbility : public UBrutusGameplayAbility
{
	GENERATED_BODY()

public:
	UBrutusClavusShieldAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float DeployDistance = 250.f;

	// How far below Clavus's feet the floor is searched for (deploying in mid-air).
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float MaxFloorDrop = 500.f;

private:
	void ServerDeploy(const FGameplayAbilityActorInfo* ActorInfo) const;
};
