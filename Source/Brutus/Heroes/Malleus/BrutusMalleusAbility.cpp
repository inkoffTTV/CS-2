#include "Heroes/Malleus/BrutusMalleusAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Brutus.h"
#include "Combat/BrutusLagCompensationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameplayPrediction.h"
#include "Heroes/Malleus/BrutusHero_Malleus.h"
#include "Raycast.h"

namespace
{
// Range of the camera trace that finds the crosshair's aim point.
constexpr float AimTraceRange = 10000.f;
}

// ---------------------------------------------------------------------------
// Target data

bool FBrutusMalleusAimTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bOriginOk = true;
	bool bDirectionOk = true;
	Origin.NetSerialize(Ar, Map, bOriginOk);
	Direction.NetSerialize(Ar, Map, bDirectionOk);
	Ar << ClientServerTime;
	bOutSuccess = bOriginOk && bDirectionOk && !Ar.IsError();
	return true;
}

// ---------------------------------------------------------------------------
// Ability

ABrutusHero_Malleus* UBrutusMalleusAbility::GetMalleus() const
{
	return Cast<ABrutusHero_Malleus>(GetBrutusCharacter());
}

bool UBrutusMalleusAbility::IsAuthority() const
{
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	return HasAuthority(&ActivationInfo);
}

void UBrutusMalleusAbility::EndSelf(bool bWasCancelled)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), IsAuthority(), bWasCancelled);
}

bool UBrutusMalleusAbility::CanDrawDebug() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_DedicatedServer;
}

void UBrutusMalleusAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (TargetDataDelegateHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ASC)
		{
			ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
		}
		TargetDataDelegateHandle.Reset();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBrutusMalleusAbility::StartAim()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const APawn* Pawn = GetBrutusCharacter();
	if (!ASC || !Pawn)
	{
		EndSelf(true);
		return;
	}

	const bool bAuthority = IsAuthority();
	if (IsLocallyControlled())
	{
		FBrutusMalleusAimTargetData* Aim = new FBrutusMalleusAimTargetData();
		const FGameplayAbilityTargetDataHandle DataHandle(Aim); // owns Aim
		if (!BuildLocalAim(*Pawn, *Aim))
		{
			EndSelf(true);
			return;
		}
		if (!bAuthority)
		{
			FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());
			ASC->CallServerSetReplicatedTargetData(GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
		}
		OnAimReady(*Aim, bAuthority, true);
		EndSelf(false);
		return;
	}

	if (!bAuthority)
	{
		EndSelf(true); // simulated proxies never run abilities; defensive
		return;
	}

	// Server, hero of a remote client: wait for its aim (it may already have arrived).
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FPredictionKey Key = GetCurrentActivationInfo().GetActivationPredictionKey();
	TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, Key).AddUObject(this, &UBrutusMalleusAbility::OnServerTargetData);
	ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, Key);
	if (IsActive())
	{
		UAbilityTask_WaitDelay* Timeout = UAbilityTask_WaitDelay::WaitDelay(this, ServerAimWaitSeconds);
		Timeout->OnFinish.AddDynamic(this, &UBrutusMalleusAbility::OnServerAimTimeout);
		Timeout->ReadyForActivation();
	}
}

void UBrutusMalleusAbility::OnServerTargetData(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag)
{
	if (!IsActive())
	{
		return;
	}
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->ConsumeClientReplicatedTargetData(GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey());
	}

	const FGameplayAbilityTargetData* Raw = Data.Data.IsValidIndex(0) ? Data.Data[0].Get() : nullptr;
	if (Raw && Raw->GetScriptStruct() == FBrutusMalleusAimTargetData::StaticStruct())
	{
		const FBrutusMalleusAimTargetData& Aim = *static_cast<const FBrutusMalleusAimTargetData*>(Raw);
		if (ValidateServerAim(Aim))
		{
			OnAimReady(Aim, true, false);
		}
	}
	EndSelf(false);
}

void UBrutusMalleusAbility::OnServerAimTimeout()
{
	if (IsActive())
	{
		UE_LOG(LogBrutus, Warning, TEXT("%s: no aim from the client, cancelled"), *GetNameSafe(GetClass()));
		EndSelf(true);
	}
}

bool UBrutusMalleusAbility::BuildLocalAim(const APawn& Pawn, FBrutusMalleusAimTargetData& Out)
{
	const AController* Controller = Pawn.GetController();
	UWorld* World = Pawn.GetWorld();
	if (!Controller || !World)
	{
		return false;
	}

	// Same aim as the weapon: towards what the camera crosshair points at, from the hero's eyes.
	FVector CamLoc;
	FRotator CamRot;
	Controller->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamEnd = CamLoc + CamRot.Vector() * AimTraceRange;

	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusMalleusAim), false, &Pawn);
	FHitResult Hit;
	const FVector AimPoint = World->LineTraceSingleByObjectType(Hit, CamLoc, CamEnd, Objects, Params) ? FVector(Hit.ImpactPoint) : CamEnd;

	const FVector Origin = Pawn.GetPawnViewLocation();
	FVector Direction = (AimPoint - Origin).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = CamRot.Vector();
	}

	const AGameStateBase* GS = World->GetGameState();
	Out.Origin = Origin;
	Out.Direction = Direction;
	Out.ClientServerTime = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return true;
}

bool UBrutusMalleusAbility::ValidateServerAim(const FBrutusMalleusAimTargetData& Aim) const
{
	const ABrutusCharacter* Hero = GetBrutusCharacter();
	if (!Hero || Aim.Origin.ContainsNaN() || Aim.Direction.ContainsNaN() || Aim.Direction.IsNearlyZero() || !FMath::IsFinite(Aim.ClientServerTime))
	{
		return false;
	}
	if (FVector::Dist(Aim.Origin, Hero->GetPawnViewLocation()) > MaxAimOriginError)
	{
		UE_LOG(LogBrutus, Warning, TEXT("%s: aim origin too far from the hero's eyes, rejected"), *GetNameSafe(Hero));
		return false;
	}
	return true;
}

double UBrutusMalleusAbility::GetRewindTime(const FBrutusMalleusAimTargetData& Aim) const
{
	const UBrutusLagCompensationSubsystem* LagComp = GetWorld() ? GetWorld()->GetSubsystem<UBrutusLagCompensationSubsystem>() : nullptr;
	const double Now = LagComp ? LagComp->GetServerTime() : Aim.ClientServerTime;
	return bsim::ClampRewindTime(Now, Aim.ClientServerTime, UBrutusLagCompensationSubsystem::MaxShotRewind);
}
