#include "Combat/BrutusHitscanWeaponComponent.h"

#include "Abilities/BrutusDamageEffect.h"
#include "Brutus.h"
#include "Character/BrutusCharacter.h"
#include "Combat/BrutusLagCompensationSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Raycast.h"

UBrutusHitscanWeaponComponent::UBrutusHitscanWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UBrutusHitscanWeaponComponent::StartFire()
{
	bTriggerHeld = true;
}

void UBrutusHitscanWeaponComponent::StopFire()
{
	bTriggerHeld = false;
}

void UBrutusHitscanWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!bTriggerHeld || !Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextLocalFireTime)
	{
		NextLocalFireTime = Now + 1.0 / FireRate;
		FireOnce();
	}
}

bool UBrutusHitscanWeaponComponent::ComputeShot(FVector& OutStart, FVector& OutDir) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Controller)
	{
		return false;
	}

	// Aim where the camera crosshair points, shoot from the hero's eyes.
	FVector CamLoc;
	FRotator CamRot;
	Controller->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamEnd = CamLoc + CamRot.Vector() * MaxRange;

	FHitResult AimHit;
	const FVector AimPoint = TraceAimable(CamLoc, CamEnd, AimHit) ? AimHit.ImpactPoint : CamEnd;

	OutStart = Pawn->GetPawnViewLocation();
	OutDir = (AimPoint - OutStart).GetSafeNormal();
	return !OutDir.IsNearlyZero();
}

void UBrutusHitscanWeaponComponent::FireOnce()
{
	FVector Start;
	FVector Dir;
	if (!ComputeShot(Start, Dir))
	{
		return;
	}

	// Cosmetic tracer right away for the shooter.
	FHitResult Hit;
	const FVector End = TraceAimable(Start, Start + Dir * MaxRange, Hit) ? Hit.ImpactPoint : Start + Dir * MaxRange;
	DrawTracer(Start, End, FColor::Yellow);

	const AGameStateBase* GS = GetWorld()->GetGameState();
	const double ClientServerTime = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	Server_Fire(Start, Dir, ClientServerTime);
}

void UBrutusHitscanWeaponComponent::Server_Fire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, double ClientServerTime)
{
	ABrutusCharacter* Shooter = Cast<ABrutusCharacter>(GetOwner());
	UBrutusLagCompensationSubsystem* LagComp = GetWorld()->GetSubsystem<UBrutusLagCompensationSubsystem>();
	if (!Shooter || !LagComp)
	{
		return;
	}

	if (!FMath::IsFinite(ClientServerTime))
	{
		return; // malformed timestamp
	}
	const double Now = LagComp->GetServerTime();
	if (LastServerTokenTime >= 0.0)
	{
		ServerShotTokens = FMath::Min(MaxShotBurst, ServerShotTokens + static_cast<float>((Now - LastServerTokenTime) * FireRate));
	}
	LastServerTokenTime = Now;
	if (ServerShotTokens < 1.f)
	{
		return; // faster than the weapon allows
	}
	if (FVector::Dist(Start, Shooter->GetPawnViewLocation()) > 200.f)
	{
		UE_LOG(LogBrutus, Warning, TEXT("%s: shot start too far from the hero's eyes, rejected"), *GetNameSafe(Shooter));
		return;
	}
	ServerShotTokens -= 1.f;

	const double RewindTime = bsim::ClampRewindTime(Now, ClientServerTime, UBrutusLagCompensationSubsystem::MaxShotRewind);
	const TOptional<FBrutusRewindHit> Hit = LagComp->RewindTrace(Start, Dir, MaxRange, RewindTime, Shooter);

	FVector End = Start + Dir * MaxRange;
	if (Hit && Hit->Target.IsValid())
	{
		End = Hit->ImpactPoint;
		ABrutusCharacter* Target = Hit->Target.Get();
		if (Target->GetTeamId() != Shooter->GetTeamId())
		{
			const bool bHead = Hit->Zone == bsim::HitZone::Head;
			UBrutusDamageEffect::Apply(Shooter->GetAbilitySystemComponent(), Target->GetAbilitySystemComponent(), Damage * (bHead ? HeadMultiplier : 1.f));
			Client_ConfirmHit(bHead);
		}
	}
	Multicast_Tracer(Start, End);
}

void UBrutusHitscanWeaponComponent::Client_ConfirmHit_Implementation(bool bHead)
{
	LastHitMarkerTime = GetWorld()->GetTimeSeconds();
	bLastHitHead = bHead;
}

void UBrutusHitscanWeaponComponent::Multicast_Tracer_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		return; // the shooter already drew it
	}
	DrawTracer(Start, End, FColor::Orange);
}

bool UBrutusHitscanWeaponComponent::TraceAimable(const FVector& From, const FVector& To, FHitResult& OutHit) const
{
	// Pawns ignore the Visibility channel, so aim against objects that include pawns.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusAim), false, GetOwner());
	return GetWorld()->LineTraceSingleByObjectType(OutHit, From, To, Objects, Params);
}

void UBrutusHitscanWeaponComponent::DrawTracer(const FVector& Start, const FVector& End, const FColor& Color) const
{
	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		DrawDebugLine(GetWorld(), Start, End, Color, false, 0.08f, 0, 1.5f);
	}
}
