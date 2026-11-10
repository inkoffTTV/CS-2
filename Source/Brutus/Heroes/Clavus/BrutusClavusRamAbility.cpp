#include "Heroes/Clavus/BrutusClavusRamAbility.h"

#include "Abilities/BrutusDamageEffect.h"
#include "Abilities/BrutusGameplayTags.h"
#include "Abilities/BrutusStunEffect.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "Character/BrutusCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Heroes/Clavus/BrutusClavusTags.h"
#include "TimerManager.h"

UBrutusClavusRamAbility::UBrutusClavusRamAbility()
{
	InputId = EBrutusAbilityInput::Ultimate;
	CooldownSeconds = 60.f;
	CooldownTag = BrutusClavusTags::Cooldown_Ram;
}

void UBrutusClavusRamAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABrutusCharacter* Hero = GetBrutusCharacter();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	HitTargets.Reset();
	// The actor follows the control yaw, which the server gets from the client's moves.
	DashDirection = FRotator(0.f, Hero->GetActorRotation().Yaw, 0.f).Vector();

	UAbilityTask_ApplyRootMotionConstantForce* Dash = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, TEXT("ClavusRam"), DashDirection, DashSpeed, DashSeconds,
		/*bIsAdditive*/ false, /*StrengthOverTime*/ nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity, /*SetVelocityOnFinish*/ FVector::ZeroVector, /*ClampVelocityOnFinish*/ FinishSpeed,
		/*bEnableGravity*/ true);
	if (!Dash)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Dash->OnFinish.AddDynamic(this, &UBrutusClavusRamAbility::OnDashFinished);
	Dash->ReadyForActivation();

	// Stunned mid-dash: stop. (Fires at once if the tag is already there, which ends the ability.)
	if (UAbilityTask_WaitGameplayTagAdded* StunWait = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, BrutusTags::State_Stunned))
	{
		StunWait->Added.AddDynamic(this, &UBrutusClavusRamAbility::OnStunned);
		StunWait->ReadyForActivation();
	}
	if (!IsActive())
	{
		return;
	}

	// Hits are decided by the server only (the client predicts just the movement and the cooldown).
	if (HasAuthority(&ActivationInfo))
	{
		ServerHitCheck();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(HitTimer, this, &UBrutusClavusRamAbility::ServerHitCheck, HitCheckInterval, true);
		}
	}
}

void UBrutusClavusRamAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimer);
	}
	HitTargets.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBrutusClavusRamAbility::OnDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBrutusClavusRamAbility::OnStunned()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UBrutusClavusRamAbility::ServerHitCheck()
{
	ABrutusCharacter* Hero = GetBrutusCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(Hero) || !SourceASC || !World || !SourceASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	const uint8 Team = Hero->GetTeamId();
	const FVector Center = Hero->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClavusRamSight), false, Hero);

	// Pick the victims first: a lethal hit destroys that hero immediately.
	TArray<TWeakObjectPtr<ABrutusCharacter>> Victims;
	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Target = *It;
		if (!IsValid(Target) || Target == Hero || HitTargets.Contains(Target))
		{
			continue;
		}
		const uint8 TargetTeam = Target->GetTeamId();
		if (TargetTeam == 255 || (Team != 255 && TargetTeam == Team))
		{
			continue; // no team yet, or an ally (friendly fire is off)
		}
		const float Reach = HitReach + Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		if (FVector::DistSquared(Center, Target->GetActorLocation()) > FMath::Square(Reach))
		{
			continue;
		}
		FHitResult Blocker;
		if (World->LineTraceSingleByObjectType(Blocker, Center, Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue; // a wall or a shield between them
		}
		HitTargets.Add(Target);
		Victims.Add(Target);
	}

	for (const TWeakObjectPtr<ABrutusCharacter>& Weak : Victims)
	{
		ABrutusCharacter* Target = Weak.Get();
		if (!IsValid(Target))
		{
			continue;
		}
		FVector Away = Target->GetActorLocation() - Center;
		Away.Z = 0.f;
		Away = Away.GetSafeNormal();
		if (Away.IsNearlyZero())
		{
			Away = DashDirection;
		}
		// Knock back and stun before the damage: the damage may kill and destroy the target.
		Target->LaunchCharacter(Away * KnockbackSpeed + FVector(0.f, 0.f, KnockbackUpSpeed), true, true);
		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		UBrutusStunEffect::Apply(SourceASC, TargetASC, StunSeconds);
		UBrutusDamageEffect::Apply(SourceASC, TargetASC, Damage);
	}
}
