#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "BrutusPlayerState.generated.h"

class UAbilitySystemComponent;
class UBrutusAttributeSet;

// Owns the hero's ability system so health, effects and (later) items survive pawn respawns.
UCLASS()
class BRUTUS_API ABrutusPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABrutusPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UBrutusAttributeSet* GetAttributes() const { return Attributes; }

	uint8 GetTeamId() const { return TeamId; }
	void SetTeamId(uint8 InTeamId) { TeamId = InTeamId; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UBrutusAttributeSet> Attributes;

	UPROPERTY(Replicated)
	uint8 TeamId = 0;
};
