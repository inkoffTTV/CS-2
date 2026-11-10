#include "Core/BrutusPlayerState.h"

#include "AbilitySystemComponent.h"
#include "Abilities/BrutusAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

ABrutusPlayerState::ABrutusPlayerState()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	Attributes = CreateDefaultSubobject<UBrutusAttributeSet>(TEXT("Attributes"));

	// Player states update rarely by default; health must feel instant.
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	SetNetUpdateFrequency(100.f);
#else
	NetUpdateFrequency = 100.f;
#endif
}

UAbilitySystemComponent* ABrutusPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void ABrutusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusPlayerState, TeamId);
	DOREPLIFETIME(ABrutusPlayerState, SelectedHero);
}
