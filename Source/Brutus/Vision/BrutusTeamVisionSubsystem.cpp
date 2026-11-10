#include "Vision/BrutusTeamVisionSubsystem.h"

#include "Character/BrutusCharacter.h"
#include "Core/BrutusSimConvert.h"
#include "Engine/World.h"
#include "EngineUtils.h"

static TAutoConsoleVariable<int32> CVarBrutusDisableVision(
	TEXT("brutus.DisableVision"), 0,
	TEXT("1: every hero is relevant to every team (debug)."));

UBrutusTeamVisionSubsystem::UBrutusTeamVisionSubsystem()
	: Vision(bsim::VisionConfig{})
{
}

bool UBrutusTeamVisionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UBrutusTeamVisionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBrutusTeamVisionSubsystem, STATGROUP_Tickables);
}

void UBrutusTeamVisionSubsystem::Tick(float DeltaTime)
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	TimeSinceUpdate += DeltaTime;
	if (TimeSinceUpdate >= UpdateInterval)
	{
		TimeSinceUpdate = 0.f;
		UpdateVision();
	}
}

void UBrutusTeamVisionSubsystem::UpdateVision()
{
	UWorld* World = GetWorld();
	TMap<int32, ABrutusCharacter*> ById;
	std::vector<bsim::VisionActor> Actors;

	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Hero = *It;
		const uint8 Team = Hero->GetTeamId();
		if (Team == 255)
		{
			continue;
		}
		const int32 Id = static_cast<int32>(Hero->GetUniqueID());
		ById.Add(Id, Hero);
		Actors.push_back(bsim::VisionActor{Id, Team, ToSim(Hero->GetActorLocation()), SightRadius, true, Hero->IsHiddenForVision()});
	}

	auto HasLineOfSight = [World, &ById](const bsim::VisionActor& ObserverInfo, const bsim::VisionActor& TargetInfo)
	{
		const ABrutusCharacter* Observer = ById.FindRef(ObserverInfo.Id);
		const ABrutusCharacter* Target = ById.FindRef(TargetInfo.Id);
		if (!Observer || !Target)
		{
			return false;
		}
		const FVector Eye = Observer->GetPawnViewLocation();
		const FVector Center = Target->GetActorLocation();
		const FVector Head = Target->GetPawnViewLocation();
		const FVector Ahead = Center + Target->GetVelocity() * LookaheadSeconds;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusVision), false, Observer);
		Params.AddIgnoredActor(Target);
		for (const FVector& Point : {Head, Center, Ahead})
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, Eye, Point, ECC_Visibility, Params))
			{
				return true;
			}
		}
		return false;
	};

	Vision.Update(World->GetTimeSeconds(), Actors, HasLineOfSight);
}

bool UBrutusTeamVisionSubsystem::IsVisibleToTeam(const AActor* Target, uint8 Team) const
{
	if (CVarBrutusDisableVision.GetValueOnAnyThread() != 0 || !Target)
	{
		return true;
	}
	return Vision.IsVisibleToTeam(static_cast<int32>(Target->GetUniqueID()), Team);
}
