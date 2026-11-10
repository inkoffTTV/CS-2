#include "Combat/BrutusHitscanWeaponComponent.h"

#include "Abilities/BrutusDamageEffect.h"
#include "Abilities/BrutusStunEffect.h"
#include "Brutus.h"
#include "Character/BrutusCharacter.h"
#include "Combat/BrutusLagCompensationSubsystem.h"
#include "Core/BrutusSimConvert.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Raycast.h"
#include "Spread.h"

UBrutusHitscanWeaponComponent::UBrutusHitscanWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SeedStream.GenerateNewSeed();
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
	if (UBrutusStunEffect::IsActorStunned(Pawn))
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

TArray<FVector> UBrutusHitscanWeaponComponent::BuildPelletDirections(const FVector& Dir, uint32 Seed) const
{
	const std::vector<bsim::Vec3> SimDirs = bsim::PelletDirections(ToSim(Dir.GetSafeNormal()), FMath::Max(0.f, SpreadDegrees), GetClampedPellets(), Seed);
	TArray<FVector> Result;
	Result.Reserve(static_cast<int32>(SimDirs.size()));
	for (const bsim::Vec3& SimDir : SimDirs)
	{
		const FVector PelletDir = ToUE(SimDir).GetSafeNormal();
		if (!PelletDir.IsNearlyZero())
		{
			Result.Add(PelletDir);
		}
	}
	return Result;
}

void UBrutusHitscanWeaponComponent::FireOnce()
{
	FVector Start;
	FVector Dir;
	if (!ComputeShot(Start, Dir))
	{
		return;
	}
	const uint32 Seed = SeedStream.GetUnsignedInt();

	// Cosmetic tracers right away for the shooter.
	for (const FVector& PelletDir : BuildPelletDirections(Dir, Seed))
	{
		FHitResult Hit;
		const FVector End = TraceAimable(Start, Start + PelletDir * MaxRange, Hit) ? Hit.ImpactPoint : Start + PelletDir * MaxRange;
		DrawTracer(Start, End, FColor::Yellow);
	}

	const AGameStateBase* GS = GetWorld()->GetGameState();
	const double ClientServerTime = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	Server_Fire(Start, Dir, ClientServerTime, Seed);
}

void UBrutusHitscanWeaponComponent::Server_Fire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, double ClientServerTime, uint32 Seed)
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
	if (UBrutusStunEffect::IsActorStunned(Shooter))
	{
		return; // stunned heroes cannot fire
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

	// Pellet damage is summed per target so each target takes one damage application per shot.
	TMap<TWeakObjectPtr<ABrutusCharacter>, float> DamageByTarget;
	bool bAnyHead = false;
	for (const FVector& PelletDir : BuildPelletDirections(Dir, Seed))
	{
		const TOptional<FBrutusRewindHit> Hit = LagComp->RewindTrace(Start, PelletDir, MaxRange, RewindTime, Shooter);
		if (!Hit || !Hit->Target.IsValid() || Hit->Target->GetTeamId() == Shooter->GetTeamId())
		{
			continue;
		}
		const bool bHead = Hit->Zone == bsim::HitZone::Head;
		bAnyHead |= bHead;
		DamageByTarget.FindOrAdd(Hit->Target) += Damage * (bHead ? HeadMultiplier : 1.f);
	}

	for (const TPair<TWeakObjectPtr<ABrutusCharacter>, float>& Pair : DamageByTarget)
	{
		if (ABrutusCharacter* Target = Pair.Key.Get())
		{
			UBrutusDamageEffect::Apply(Shooter->GetAbilitySystemComponent(), Target->GetAbilitySystemComponent(), Pair.Value);
		}
	}
	if (DamageByTarget.Num() > 0)
	{
		Client_ConfirmHit(bAnyHead); // one hit marker per shot
	}
	Multicast_Tracer(Start, Dir, Seed);
}

void UBrutusHitscanWeaponComponent::Client_ConfirmHit_Implementation(bool bHead)
{
	LastHitMarkerTime = GetWorld()->GetTimeSeconds();
	bLastHitHead = bHead;
}

void UBrutusHitscanWeaponComponent::Multicast_Tracer_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint32 Seed)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if ((Pawn && Pawn->IsLocallyControlled()) || GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return; // the shooter already drew it; nothing to draw on a dedicated server
	}
	for (const FVector& PelletDir : BuildPelletDirections(Dir, Seed))
	{
		FHitResult Hit;
		const FVector End = TraceAimable(Start, Start + PelletDir * MaxRange, Hit) ? Hit.ImpactPoint : Start + PelletDir * MaxRange;
		DrawTracer(Start, End, FColor::Orange);
	}
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
