#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Heroes/BrutusHeroRegistry.h"
#include "BrutusGameMode.generated.h"

class ABrutusCharacter;

UCLASS()
class BRUTUS_API ABrutusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABrutusGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual bool ShouldSpawnAtStartSpot_Implementation(AController* Player) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	// Pawn class of the player's selected hero (BrutusHeroRegistry).
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// Server: removes the dead hero and respawns its owner after RespawnDelay.
	void HandleHeroDeath(ABrutusCharacter* Victim, AController* Killer);

	// Server: selects Hero for Controller. A living hero is removed without counting a death and the
	// player respawns at once as the new hero; a dead player respawns as it when the timer runs out.
	void ChangeHero(AController* Controller, EBrutusHero Hero);

	// Starting value for tuning; the design grows it over the match later.
	UPROPERTY(EditDefaultsOnly, Category = "Brutus")
	float RespawnDelay = 5.f;

private:
	void RespawnHero(AController* Controller);
	uint8 PickTeamForNewPlayer(const APlayerState* NewPlayerState) const;

	// Controllers waiting for the respawn timer.
	TSet<TWeakObjectPtr<AController>> PendingRespawns;
};
