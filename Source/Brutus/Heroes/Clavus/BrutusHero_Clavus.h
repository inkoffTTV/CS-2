#pragma once

#include "CoreMinimal.h"
#include "Character/BrutusCharacter.h"
#include "BrutusHero_Clavus.generated.h"

class UStaticMeshComponent;

// Clavus ("nail"): mid-range all-rounder with a nailgun.
// Q sticky grenade, F deployable shield, R Ram.
// The class name is the registry key: /Script/Brutus.BrutusHero_Clavus.
UCLASS()
class BRUTUS_API ABrutusHero_Clavus : public ABrutusCharacter
{
	GENERATED_BODY()

public:
	explicit ABrutusHero_Clavus(const FObjectInitializer& ObjectInitializer);

	virtual TArray<TSubclassOf<UBrutusGameplayAbility>> GetHeroAbilities() const override;
	virtual void ConfigureWeapon(UBrutusHitscanWeaponComponent& InWeapon) override;

	virtual void PostInitializeComponents() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

private:
	// Keeps the shoulder pad on the right shoulder of the current capsule (after resize or crouch).
	void FitShoulderPad();

	// Greybox marker: a box on the right shoulder so Clavus reads differently from the plain cylinder.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ShoulderPad;
};
