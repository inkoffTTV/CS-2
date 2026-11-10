#include "Heroes/Clavus/BrutusClavusStickyGrenadeAbility.h"

#include "Character/BrutusCharacter.h"
#include "Engine/World.h"
#include "Heroes/Clavus/BrutusClavusStickyGrenade.h"
#include "Heroes/Clavus/BrutusClavusTags.h"

UBrutusClavusStickyGrenadeAbility::UBrutusClavusStickyGrenadeAbility()
{
	InputId = EBrutusAbilityInput::Ability1;
	CooldownSeconds = 12.f;
	CooldownTag = BrutusClavusTags::Cooldown_StickyGrenade;
	GrenadeClass = ABrutusClavusStickyGrenade::StaticClass();
}

void UBrutusClavusStickyGrenadeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Predicted on the owning client (cooldown only); the grenade exists only once, on the server.
	if (HasAuthority(&ActivationInfo))
	{
		ServerThrow(ActorInfo);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UBrutusClavusStickyGrenadeAbility::ServerThrow(const FGameplayAbilityActorInfo* ActorInfo) const
{
	ABrutusCharacter* Hero = ActorInfo ? Cast<ABrutusCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UWorld* World = Hero ? Hero->GetWorld() : nullptr;
	if (!Hero || !World || !GrenadeClass)
	{
		return;
	}

	// On the server the control rotation follows the client's moves (view yaw and pitch).
	FRotator Aim = Hero->GetControlRotation();
	Aim.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch) + static_cast<double>(ThrowPitchUpDegrees), -89.0, 89.0);
	const FVector Dir = Aim.Vector();

	// Spawn just in front of the eyes, but never on the far side of a wall Clavus is hugging.
	const FVector Eyes = Hero->GetPawnViewLocation();
	FVector SpawnLocation = Eyes + Dir * SpawnForwardOffset;
	FHitResult WallHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClavusGrenadeSpawn), false, Hero);
	if (World->LineTraceSingleByObjectType(WallHit, Eyes, SpawnLocation, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		SpawnLocation = WallHit.Location - Dir * 15.f;
	}

	ABrutusProjectile::SpawnProjectile(TSubclassOf<ABrutusProjectile>(GrenadeClass.Get()), Hero, SpawnLocation, Dir * ThrowSpeed);
}
