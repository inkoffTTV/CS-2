#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BrutusGameMode.generated.h"

UCLASS()
class BRUTUS_API ABrutusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABrutusGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	uint8 PickTeamForNewPlayer(const APlayerState* NewPlayerState) const;
};
