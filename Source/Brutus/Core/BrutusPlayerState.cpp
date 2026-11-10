#include "Core/BrutusPlayerState.h"

#include "Net/UnrealNetwork.h"

void ABrutusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusPlayerState, TeamId);
}
