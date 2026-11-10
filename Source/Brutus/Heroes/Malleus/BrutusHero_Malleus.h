#pragma once

#include "CoreMinimal.h"
#include "Character/BrutusCharacter.h"
#include "BrutusHero_Malleus.generated.h"

class UStaticMeshComponent;

// How Rage enlarges Malleus.
UENUM()
enum class EBrutusMalleusRageScale : uint8
{
	None,
	// Only the visuals grow (there was no room for a bigger capsule when Rage started).
	VisualOnly,
	// Visuals, collision capsule and hitboxes grow (the whole actor is scaled).
	Full,
};

// Malleus: tank / brawler. Big capsule, shotgun, hammer swing, ground slam, chain pull, Rage.
// Resolved by BrutusHeroRegistry as "/Script/Brutus.BrutusHero_Malleus".
UCLASS()
class BRUTUS_API ABrutusHero_Malleus : public ABrutusCharacter
{
	GENERATED_BODY()

public:
	explicit ABrutusHero_Malleus(const FObjectInitializer& ObjectInitializer);

	// Starting value for tuning.
	static constexpr float RageScaleFactor = 1.3f;

	// Rage size. Server: authoritative (replicated). Owning client: predicted until the server's value arrives.
	void SetRageActive(bool bActive);
	bool IsRaging() const { return AppliedRageScale != EBrutusMalleusRageScale::None; }

	virtual TArray<TSubclassOf<UBrutusGameplayAbility>> GetHeroAbilities() const override;
	virtual void ConfigureWeapon(UBrutusHitscanWeaponComponent& InWeapon) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

private:
	// Places the greybox hammer and shoulders relative to the current capsule.
	void FitGearToCapsule();

	// Full when a capsule RageScaleFactor times bigger (raised to keep the feet on the floor) fits here.
	EBrutusMalleusRageScale ChooseRageScale() const;
	void ApplyRageScale(EBrutusMalleusRageScale NewScale);
	// Height the capsule centre moves when the actor scale changes between 1 and RageScaleFactor.
	float GetRageLift() const;

	UFUNCTION()
	void OnRep_RageScale();

	UPROPERTY(ReplicatedUsing = OnRep_RageScale)
	EBrutusMalleusRageScale RageScale = EBrutusMalleusRageScale::None;

	// What this machine currently shows (may be a client prediction ahead of RageScale).
	EBrutusMalleusRageScale AppliedRageScale = EBrutusMalleusRageScale::None;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> HammerHead;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> HammerHandle;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Shoulders;
};
