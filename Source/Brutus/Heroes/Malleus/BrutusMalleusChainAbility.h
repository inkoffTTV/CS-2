#pragma once

#include "CoreMinimal.h"
#include "Heroes/Malleus/BrutusMalleusAbility.h"
#include "BrutusMalleusChainAbility.generated.h"

// F: chain pull. Lag-compensated trace along the aim; the first enemy hero hit (walls block, teammates
// do not) takes Damage and is launched so it lands PullDistance in front of Malleus after PullFlightTime.
UCLASS()
class BRUTUS_API UBrutusMalleusChainAbility : public UBrutusMalleusAbility
{
	GENERATED_BODY()

public:
	UBrutusMalleusChainAbility();

	// Starting values for tuning.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Range = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Damage = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float PullDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float PullFlightTime = 0.45f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnAimReady(const FBrutusMalleusAimTargetData& Aim, bool bAuthority, bool bLocal) override;

private:
	// Server: launches Target towards the point in front of Hero.
	void Pull(ABrutusCharacter& Hero, ABrutusCharacter& Target, const FVector& Forward) const;
};
