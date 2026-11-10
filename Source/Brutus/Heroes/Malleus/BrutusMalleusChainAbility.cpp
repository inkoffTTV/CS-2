#include "Heroes/Malleus/BrutusMalleusChainAbility.h"

#include "Abilities/BrutusDamageEffect.h"
#include "Ballistics.h"
#include "Character/BrutusCharacter.h"
#include "Combat/BrutusLagCompensationSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Core/BrutusSimConvert.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Heroes/Malleus/BrutusMalleusTags.h"

UBrutusMalleusChainAbility::UBrutusMalleusChainAbility()
{
	InputId = EBrutusAbilityInput::Ability2;
	CooldownSeconds = 16.f;
	CooldownTag = BrutusMalleusTags::Cooldown_Chain;
}

void UBrutusMalleusChainAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartAim();
}

void UBrutusMalleusChainAbility::OnAimReady(const FBrutusMalleusAimTargetData& Aim, bool bAuthority, bool bLocal)
{
	ABrutusCharacter* Hero = GetBrutusCharacter();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return;
	}
	const FVector Start = Aim.Origin;
	const FVector Dir = Aim.Direction.GetSafeNormal();

	if (bLocal && CanDrawDebug())
	{
		DrawDebugLine(World, Start, Start + Dir * Range, FColor(160, 160, 160), false, 0.3f, 0, 3.f);
	}
	if (!bAuthority)
	{
		return;
	}

	// Enemy heroes as the client saw them; teammates are skipped and WorldStatic walls in between block.
	const UBrutusLagCompensationSubsystem* LagComp = World->GetSubsystem<UBrutusLagCompensationSubsystem>();
	const TOptional<FBrutusRewindHit> Hit = LagComp ? LagComp->RewindTrace(Start, Dir, Range, GetRewindTime(Aim), Hero) : TOptional<FBrutusRewindHit>();
	ABrutusCharacter* Target = Hit ? Hit->Target.Get() : nullptr;
	if (!Target || Target->GetTeamId() == Hero->GetTeamId())
	{
		return;
	}

	FVector Forward = FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Hero->GetActorForwardVector().GetSafeNormal2D();
	}
	if (CanDrawDebug())
	{
		DrawDebugLine(World, Start, Target->GetActorLocation(), FColor::Orange, false, 0.5f, 0, 5.f);
	}

	// Pull before damage: the damage may kill (and destroy) the target.
	Pull(*Hero, *Target, Forward);
	UBrutusDamageEffect::Apply(Hero->GetAbilitySystemComponent(), Target->GetAbilitySystemComponent(), Damage);
}

void UBrutusMalleusChainAbility::Pull(ABrutusCharacter& Hero, ABrutusCharacter& Target, const FVector& Forward) const
{
	UCharacterMovementComponent* TargetMove = Target.GetCharacterMovement();
	UWorld* World = GetWorld();
	if (!TargetMove || !World)
	{
		return;
	}

	const FVector HeroLocation = Hero.GetActorLocation();
	const FVector TargetLocation = Target.GetActorLocation();
	const float TargetRadius = Target.GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float TargetHalfHeight = Target.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// Already this close in front: nothing to pull.
	if (FVector::DistXY(HeroLocation, TargetLocation) <= PullDistance)
	{
		return;
	}

	// Destination: PullDistance in front of Malleus, target's feet at Malleus's feet level.
	const float FeetZ = HeroLocation.Z - Hero.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector From(HeroLocation.X, HeroLocation.Y, FeetZ + TargetHalfHeight);
	FVector Destination = From + Forward * PullDistance;

	// A wall right in front of Malleus: stop the destination short of it instead of aiming into it.
	FHitResult WallHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusMalleusChainPull), false, &Hero);
	Params.AddIgnoredActor(&Target);
	if (World->SweepSingleByObjectType(WallHit, From, Destination, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeCapsule(TargetRadius, TargetHalfHeight), Params)
		&& !WallHit.bStartPenetrating)
	{
		Destination = WallHit.Location;
	}

	// Off ziplines and out of slides first, or the custom movement mode would keep the target in place.
	// The launch itself also switches to falling, but only when it is processed on the target's next move.
	TargetMove->SetMovementMode(MOVE_Falling);

	const bsim::Vec3 Velocity = bsim::LaunchVelocityToReach(ToSim(TargetLocation), ToSim(Destination), TargetMove->GetGravityZ(), PullFlightTime);
	Target.LaunchCharacter(ToUE(Velocity), true, true);
}
