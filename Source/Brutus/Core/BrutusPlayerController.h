#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Heroes/BrutusHeroRegistry.h"
#include "BrutusPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UENUM()
enum class EBrutusInput : uint8
{
	Move,
	Look,
	Jump,
	Dash,
	Slide,
	Fire,
	Interact,
	Ability1, // Q
	Ability2, // F
	Ultimate, // R
	AltFire,  // right mouse button
	Count UMETA(Hidden)
};

// Builds Enhanced Input actions and key mappings at runtime, so the project needs no input assets.
UCLASS()
class BRUTUS_API ABrutusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	const UInputAction* GetAction(EBrutusInput Input) const;

	// Switch to another hero: the current hero is removed (not counted as a death) and the player
	// respawns at once as the new one. Console: brutus.Hero Clavus|Malleus
	UFUNCTION(Server, Reliable)
	void Server_SelectHero(EBrutusHero Hero);

private:
	void BuildInput();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> Actions;
};
