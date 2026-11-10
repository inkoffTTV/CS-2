#pragma once

#include "CoreMinimal.h"
#include "Heroes/Malleus/BrutusMalleusAbility.h"
#include "BrutusMalleusGroundSlamAbility.generated.h"

// Q: ground slam. Malleus leaps up (horizontal speed kept); when he lands the server damages enemies
// around him (radial falloff) and slows them. The ability ends on landing, or after MaxAirTime.
UCLASS()
class BRUTUS_API UBrutusMalleusGroundSlamAbility : public UBrutusMalleusAbility
{
	GENERATED_BODY()

public:
	UBrutusMalleusGroundSlamAbility();

	// Starting values for tuning.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float LaunchZ = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Damage = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Radius = 400.f;

	// Damage fraction at the edge of the radius.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float MinFalloff = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float MaxAirTime = 3.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

	UFUNCTION()
	void OnAirTimeout();

	void Slam(ABrutusCharacter& Hero);
	void UnbindLanded();

	TWeakObjectPtr<ABrutusCharacter> BoundHero;
};
