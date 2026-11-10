#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BrutusGameState.generated.h"

// Server world time (GetServerWorldTimeSeconds) is what clients stamp their shots with.
UCLASS()
class BRUTUS_API ABrutusGameState : public AGameStateBase
{
	GENERATED_BODY()
};
