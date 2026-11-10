#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BrutusPlayerState.generated.h"

UCLASS()
class BRUTUS_API ABrutusPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	uint8 GetTeamId() const { return TeamId; }
	void SetTeamId(uint8 InTeamId) { TeamId = InTeamId; }

private:
	UPROPERTY(Replicated)
	uint8 TeamId = 0;
};
