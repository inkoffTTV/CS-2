#include "Core/BrutusGameMode.h"

#include "Brutus.h"
#include "Abilities/BrutusDamageEffect.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Character/BrutusCharacter.h"
#include "Core/BrutusGameState.h"
#include "Core/BrutusHUD.h"
#include "Core/BrutusPlayerController.h"
#include "Core/BrutusPlayerState.h"
#include "Core/BrutusTeams.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

ABrutusGameMode::ABrutusGameMode()
{
	PlayerControllerClass = ABrutusPlayerController::StaticClass();
	PlayerStateClass = ABrutusPlayerState::StaticClass();
	GameStateClass = ABrutusGameState::StaticClass();
	HUDClass = ABrutusHUD::StaticClass();
	DefaultPawnClass = ABrutusCharacter::StaticClass();
}

FString ABrutusGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Super chooses the start spot, so the team must be known first.
	if (ABrutusPlayerState* PS = NewPlayerController ? NewPlayerController->GetPlayerState<ABrutusPlayerState>() : nullptr)
	{
		PS->SetTeamId(PickTeamForNewPlayer(PS));
		UE_LOG(LogBrutus, Log, TEXT("New player joined the %s"), BrutusTeams::GetTeamName(PS->GetTeamId()));
	}
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

bool ABrutusGameMode::ShouldSpawnAtStartSpot_Implementation(AController* Player)
{
	// Always pick a fresh start of the player's team (also on respawn).
	return false;
}

uint8 ABrutusGameMode::PickTeamForNewPlayer(const APlayerState* NewPlayerState) const
{
	int32 Counts[2] = {0, 0};
	for (const APlayerState* PS : GameState->PlayerArray)
	{
		if (const ABrutusPlayerState* BPS = Cast<ABrutusPlayerState>(PS))
		{
			if (BPS != NewPlayerState && BPS->GetTeamId() < 2)
			{
				++Counts[BPS->GetTeamId()];
			}
		}
	}
	return Counts[1] < Counts[0] ? 1 : 0;
}

AActor* ABrutusGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const ABrutusPlayerState* PS = Player ? Player->GetPlayerState<ABrutusPlayerState>() : nullptr;
	if (PS)
	{
		const FName Tag(*FString::Printf(TEXT("Team%d"), PS->GetTeamId()));
		TArray<APlayerStart*> Starts;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == Tag)
			{
				Starts.Add(*It);
			}
		}
		if (Starts.Num() > 0)
		{
			return Starts[FMath::RandRange(0, Starts.Num() - 1)];
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

UClass* ABrutusGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ABrutusPlayerState* PS = InController ? InController->GetPlayerState<ABrutusPlayerState>() : nullptr)
	{
		return BrutusHeroRegistry::GetHeroClass(PS->GetSelectedHero()).Get();
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ABrutusGameMode::HandleHeroDeath(ABrutusCharacter* Victim, AController* Killer)
{
	if (!Victim)
	{
		return;
	}
	AController* VictimController = Victim->GetController();
	UE_LOG(LogBrutus, Log, TEXT("%s killed by %s"), *GetNameSafe(VictimController), *GetNameSafe(Killer));

	Victim->DetachFromControllerPendingDestroy();
	Victim->Destroy();

	if (!VictimController)
	{
		return;
	}
	TWeakObjectPtr<AController> WeakController = VictimController;
	PendingRespawns.Add(WeakController);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, WeakController]()
	{
		PendingRespawns.Remove(WeakController);
		AController* Controller = WeakController.Get();
		if (Controller && !Controller->GetPawn())
		{
			RespawnHero(Controller);
		}
	}), RespawnDelay, false);
}

void ABrutusGameMode::ChangeHero(AController* Controller, EBrutusHero Hero)
{
	ABrutusPlayerState* PS = Controller ? Controller->GetPlayerState<ABrutusPlayerState>() : nullptr;
	if (!PS || !BrutusHeroRegistry::IsValidHero(Hero))
	{
		return;
	}
	if (PS->GetSelectedHero() == Hero)
	{
		return; // already this hero (or already queued for the next respawn)
	}
	PS->SetSelectedHero(Hero);
	PS->bResetCooldownsOnNextSpawn = true;
	UE_LOG(LogBrutus, Log, TEXT("%s selected hero %s"), *GetNameSafe(Controller), *BrutusHeroRegistry::GetHeroName(Hero));

	if (APawn* OldPawn = Controller->GetPawn())
	{
		// Away from base the pick waits for the next respawn.
		const FName TeamTag(*FString::Printf(TEXT("Team%d"), PS->GetTeamId()));
		bool bNearOwnSpawn = false;
		for (TActorIterator<APlayerStart> It(GetWorld()); It && !bNearOwnSpawn; ++It)
		{
			bNearOwnSpawn = It->PlayerStartTag == TeamTag && FVector::Dist(It->GetActorLocation(), OldPawn->GetActorLocation()) <= HeroSwitchRadius;
		}
		if (!bNearOwnSpawn)
		{
			UE_LOG(LogBrutus, Log, TEXT("%s: hero switch applies on the next respawn"), *GetNameSafe(Controller));
			return;
		}
		// Removed without going through HandleHeroDeath: a hero switch is not a death.
		OldPawn->DetachFromControllerPendingDestroy();
		OldPawn->Destroy();
	}
	else if (PendingRespawns.Contains(TWeakObjectPtr<AController>(Controller)))
	{
		return; // dead: the pending respawn will use the new hero
	}
	RespawnHero(Controller);
}

void ABrutusGameMode::RespawnHero(AController* Controller)
{
	// Health, attributes, effects and abilities are reset by the new hero in ABrutusCharacter::PossessedBy.
	RestartPlayer(Controller);
}

// Debug: brutus.DamageSelf <amount> — damages the local hero on a listen server host.
static FAutoConsoleCommandWithWorldAndArgs GBrutusDamageSelf(
	TEXT("brutus.DamageSelf"),
	TEXT("brutus.DamageSelf <amount>: damage your own hero (server/host only)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogBrutus, Warning, TEXT("brutus.DamageSelf runs on the server/host only."));
			return;
		}
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 50.f;
		APlayerController* PC = World->GetFirstPlayerController();
		ABrutusPlayerState* PS = PC ? PC->GetPlayerState<ABrutusPlayerState>() : nullptr;
		if (PS)
		{
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UBrutusDamageEffect::Apply(ASC, ASC, Amount);
		}
	}));
