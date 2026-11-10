#pragma once

#include "CoreMinimal.h"
#include "Abilities/BrutusGameplayAbility.h"
#include "BrutusClavusRamAbility.generated.h"

class ABrutusCharacter;

// Clavus R "Ram": a DashSpeed dash forward for DashSeconds through a network-predicted root motion
// source (the owning client and the server run the same task). On the server, every HitCheckInterval
// during the dash, enemies within HitReach of Clavus's capsule take Damage, are knocked away and up
// and are stunned for StunSeconds; each enemy at most once per activation.
// The dash stops early if Clavus gets stunned.
UCLASS()
class BRUTUS_API UBrutusClavusRamAbility : public UBrutusGameplayAbility
{
	GENERATED_BODY()

public:
	UBrutusClavusRamAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float DashSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float DashSeconds = 0.6f;

	// Speed Clavus keeps (at most) when the dash ends.
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float FinishSpeed = 600.f;

	// Gap between Clavus's centre and the target's capsule surface that counts as a hit
	// (about 120 cm centre to centre against a base-size hero).
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float HitReach = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float Damage = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float StunSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float KnockbackSpeed = 1300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float KnockbackUpSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float HitCheckInterval = 1.f / 30.f;

private:
	UFUNCTION()
	void OnDashFinished();

	UFUNCTION()
	void OnStunned();

	// Server.
	void ServerHitCheck();

	FVector DashDirection = FVector::ForwardVector;
	TSet<TWeakObjectPtr<ABrutusCharacter>> HitTargets;
	FTimerHandle HitTimer;
};
