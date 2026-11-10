#include "Core/BrutusGameMode.h"

#include "Brutus.h"
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
