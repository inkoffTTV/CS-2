#include "Core/BrutusGameMode.h"

#include "Brutus.h"
#include "Abilities/BrutusAttributeSet.h"
#include "Abilities/BrutusDamageEffect.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Character/BrutusCharacter.h"
#include "Core/BrutusGameState.h"
#include "Core/BrutusHUD.h"
#include "Core/BrutusPlayerController.h"
#include "Core/BrutusPlayerState.h"
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

void ABrutusGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Team must be known before Super spawns the pawn at a team start.
	if (ABrutusPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ABrutusPlayerState>() : nullptr)
	{
		PS->SetTeamId(PickTeamForNewPlayer(PS));
		UE_LOG(LogBrutus, Log, TEXT("%s joined team %d"), *PS->GetPlayerName(), PS->GetTeamId());
	}
	Super::PostLogin(NewPlayer);
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
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, WeakController]()
	{
		if (AController* Controller = WeakController.Get())
		{
			RespawnHero(Controller);
		}
	}), RespawnDelay, false);
}

void ABrutusGameMode::RespawnHero(AController* Controller)
{
	if (ABrutusPlayerState* PS = Controller->GetPlayerState<ABrutusPlayerState>())
	{
		UBrutusAttributeSet* Attributes = PS->GetAttributes();
		PS->GetAbilitySystemComponent()->SetNumericAttributeBase(UBrutusAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
		Attributes->ResetOutOfHealth();
	}
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
