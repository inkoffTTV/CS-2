#include "Combat/BrutusLagCompensationSubsystem.h"

#include "Character/BrutusCharacter.h"
#include "Character/BrutusHitboxComponent.h"
#include "Core/BrutusSimConvert.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Raycast.h"

static TAutoConsoleVariable<int32> CVarBrutusDebugLagComp(
	TEXT("brutus.DebugLagComp"), 0,
	TEXT("1: draw rewound hitboxes (green) and current hitboxes (red) for every server shot."));

namespace
{
void DrawCapsules(const UWorld* World, const std::vector<bsim::Capsule>& Shapes, const FColor& Color)
{
	for (const bsim::Capsule& C : Shapes)
	{
		const FVector A = ToUE(C.A);
		const FVector B = ToUE(C.B);
		const float HalfHeight = (B - A).Size() * 0.5f + C.Radius;
		DrawDebugCapsule(World, (A + B) * 0.5f, HalfHeight, C.Radius, FQuat::Identity, Color, false, 2.f);
	}
}
} // namespace

bool UBrutusLagCompensationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UBrutusLagCompensationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBrutusLagCompensationSubsystem, STATGROUP_Tickables);
}

double UBrutusLagCompensationSubsystem::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return GS ? GS->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

void UBrutusLagCompensationSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	const double Now = GetServerTime();

	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Hero = *It;
		const UBrutusHitboxComponent* Hitbox = Hero->FindComponentByClass<UBrutusHitboxComponent>();
		if (!Hitbox)
		{
			continue;
		}
		const TWeakObjectPtr<ABrutusCharacter> Key(Hero);
		TUniquePtr<bsim::HitboxHistory>* Found = Histories.Find(Key);
		if (!Found)
		{
			Found = &Histories.Add(Key, MakeUnique<bsim::HitboxHistory>(HistorySeconds));
			Hero->OnTeleported.AddUObject(this, &UBrutusLagCompensationSubsystem::OnHeroTeleported);
		}
		bsim::HitboxHistory* History = Found->Get();
		const bool bTeleported = PendingTeleports.Remove(Key) > 0;
		History->Record(Now, Hitbox->BuildWorldCapsules(), bTeleported);
		History->Prune(Now);
	}

	for (auto It = Histories.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UBrutusLagCompensationSubsystem::OnHeroTeleported(ABrutusCharacter* Hero)
{
	PendingTeleports.Add(Hero);
}

TOptional<FBrutusRewindHit> UBrutusLagCompensationSubsystem::RewindTrace(const FVector& Start, const FVector& Dir, float MaxRange, double ShotServerTime, const AActor* Shooter) const
{
	const UWorld* World = GetWorld();
	const bsim::Ray Ray{ToSim(Start), ToSim(Dir.GetSafeNormal())};
	const bool bDebug = CVarBrutusDebugLagComp.GetValueOnGameThread() != 0;

	TOptional<FBrutusRewindHit> Best;
	for (const TPair<TWeakObjectPtr<ABrutusCharacter>, TUniquePtr<bsim::HitboxHistory>>& Pair : Histories)
	{
		ABrutusCharacter* Hero = Pair.Key.Get();
		if (!Hero || Hero == Shooter)
		{
			continue;
		}
		const std::optional<std::vector<bsim::Capsule>> Shapes = Pair.Value->SampleAt(ShotServerTime);
		if (!Shapes)
		{
			continue;
		}
		if (bDebug)
		{
			DrawCapsules(World, *Shapes, FColor::Green);
			if (const UBrutusHitboxComponent* Hitbox = Hero->FindComponentByClass<UBrutusHitboxComponent>())
			{
				DrawCapsules(World, Hitbox->BuildWorldCapsules(), FColor::Red);
			}
		}
		const std::optional<bsim::RayHit> Hit = bsim::RaycastCapsules(Ray, *Shapes, MaxRange);
		if (Hit && (!Best || Hit->Distance < Best->Distance))
		{
			FBrutusRewindHit Result;
			Result.Target = Hero;
			Result.Distance = Hit->Distance;
			Result.Zone = Hit->Zone;
			Result.ImpactPoint = Start + Dir.GetSafeNormal() * Hit->Distance;
			Best = Result;
		}
	}

	// Walls do not move, so occlusion is checked against the current static world.
	if (Best && World)
	{
		FHitResult WallHit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusRewindOcclusion), false, Shooter);
		if (World->LineTraceSingleByObjectType(WallHit, Start, Best->ImpactPoint, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Best.Reset();
		}
	}
	return Best;
}
