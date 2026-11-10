#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Heroes/Malleus/BrutusMalleusAbility.h"
#include "BrutusMalleusRageAbility.generated.h"

// R: Rage. For Duration seconds Malleus takes less damage (UBrutusMalleusRageEffect) and is RageScaleFactor
// times bigger (ABrutusHero_Malleus::SetRageActive); he heals Heal at once.
UCLASS()
class BRUTUS_API UBrutusMalleusRageAbility : public UBrutusMalleusAbility
{
	GENERATED_BODY()

public:
	UBrutusMalleusRageAbility();

	// Starting values for tuning.
	UPROPERTY(EditDefaultsOnly, Category = "Malleus")
	float Heal = 300.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnRageFinished();

	FActiveGameplayEffectHandle RageEffectHandle;
	TWeakObjectPtr<ABrutusHero_Malleus> RagingHero;
};
