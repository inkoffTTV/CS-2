#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BrutusGameMode.generated.h"

class ABrutusCharacter;

UCLASS()
class BRUTUS_API ABrutusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABrutusGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// Server: removes the dead hero and respawns its owner after RespawnDelay.
	void HandleHeroDeath(ABrutusCharacter* Victim, AController* Killer);

	// Starting value for tuning; the design grows it over the match later.
	UPROPERTY(EditDefaultsOnly, Category = "Brutus")
	float RespawnDelay = 5.f;

private:
	void RespawnHero(AController* Controller);
	uint8 PickTeamForNewPlayer(const APlayerState* NewPlayerState) const;
};
