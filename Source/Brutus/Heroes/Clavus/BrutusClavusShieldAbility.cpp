#include "Heroes/Clavus/BrutusClavusShieldAbility.h"

#include "Character/BrutusCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Heroes/Clavus/BrutusClavusShield.h"
#include "Heroes/Clavus/BrutusClavusTags.h"

UBrutusClavusShieldAbility::UBrutusClavusShieldAbility()
{
	InputId = EBrutusAbilityInput::Ability2;
	CooldownSeconds = 20.f;
	CooldownTag = BrutusClavusTags::Cooldown_Shield;
}

void UBrutusClavusShieldAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Predicted on the owning client (cooldown only); only the server spawns the wall.
	if (HasAuthority(&ActivationInfo))
	{
		ServerDeploy(ActorInfo);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UBrutusClavusShieldAbility::ServerDeploy(const FGameplayAbilityActorInfo* ActorInfo) const
{
	ABrutusCharacter* Hero = ActorInfo ? Cast<ABrutusCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UWorld* World = Hero ? Hero->GetWorld() : nullptr;
	if (!Hero || !World)
	{
		return;
	}

	const float Yaw = Hero->GetActorRotation().Yaw;
	const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
	const FVector Center = Hero->GetActorLocation();
	const float HalfHeight = Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClavusShieldPlacement), false, Hero);
	const FCollisionObjectQueryParams WorldStatic(ECC_WorldStatic);

	// Never through a wall Clavus is facing: stop just in front of it.
	float Distance = DeployDistance;
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Center, Center + Forward * DeployDistance, WorldStatic, Params))
	{
		Distance = FMath::Max(0.f, Hit.Distance - (ABrutusClavusShield::Thickness * 0.5f + 5.f));
	}
	const FVector Spot = Center + Forward * Distance;

	// Stand it on the floor under that spot (at Clavus's feet height if there is none).
	FVector Floor(Spot.X, Spot.Y, Center.Z - HalfHeight);
	if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 50.f), Spot - FVector(0.f, 0.f, HalfHeight + MaxFloorDrop), WorldStatic, Params))
	{
		Floor = Hit.ImpactPoint;
	}

	ABrutusClavusShield::SpawnShield(Hero, Floor, Yaw);
}
