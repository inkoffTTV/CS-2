#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Abilities/BrutusGameplayAbility.h"
#include "GameFramework/Character.h"
#include "BrutusCharacter.generated.h"

class UBrutusMovementComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UBrutusHitboxComponent;
class UBrutusHitscanWeaponComponent;
struct FInputActionValue;
class ABrutusPlayerState;
class ABrutusCharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroTeleported, ABrutusCharacter*);

// Greybox hero: capsule + cylinder, over-the-shoulder camera, Brutus movement.
// Heroes subclass it (see BrutusHeroRegistry.h for naming): set the capsule and HeroMaxHealth in the
// constructor, override GetHeroAbilities() and ConfigureWeapon().
UCLASS()
class BRUTUS_API ABrutusCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	explicit ABrutusCharacter(const FObjectInitializer& ObjectInitializer);

	UBrutusMovementComponent* GetBrutusMovement() const;
	ABrutusPlayerState* GetBrutusPlayerState() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 255 when the hero has no player state yet.
	uint8 GetTeamId() const;

	// Fired on the server when the hero is placed somewhere without travelling (spawn, teleport).
	// Lag compensation uses it to avoid interpolating across the jump.
	FOnHeroTeleported OnTeleported;
	void NotifyTeleported() { OnTeleported.Broadcast(this); }

	UBrutusHitscanWeaponComponent* GetWeapon() const { return Weapon; }

	// Ability classes granted (with their InputId) when the hero is possessed. Server.
	virtual TArray<TSubclassOf<UBrutusGameplayAbility>> GetHeroAbilities() const { return {}; }

	// Tunes the weapon for this hero. Called on every machine from PostInitializeComponents.
	virtual void ConfigureWeapon(UBrutusHitscanWeaponComponent& InWeapon) {}

	// Max health set on the player's attribute set each time this hero is possessed.
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	float HeroMaxHealth = 250.f;

	// Invisible heroes (Nemo) are only revealed to enemies up close.
	bool IsHiddenForVision() const { return bHiddenForVision; }
	void SetHiddenForVision(bool bHidden) { bHiddenForVision = bHidden; }

	// Enemies the viewer's team cannot see are not replicated to that viewer.
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

protected:
	virtual bool CanJumpInternal_Implementation() const override;

	// Posed static placeholder model (Art/Export/SM_Hero_<Name>_Posed.fbx imported into the project).
	// When the asset exists it replaces every component tagged GreyboxTag; otherwise the greybox stays.
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FSoftObjectPath PlaceholderMeshPath;

	static const FName GreyboxTag;

	// Scales the greybox cylinder to the current capsule (call after resizing the capsule).
	void FitBodyMeshToCapsule();

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpCompleted();
	void OnDash();
	void OnSlideStarted();
	void OnSlideCompleted();
	void OnInteract();
	void OnFireStarted();
	void OnFireCompleted();
	void OnAbilityInputPressed(EBrutusAbilityInput InputId);
	void OnAbilityInputReleased(EBrutusAbilityInput InputId);

	void InitAbilitySystem();
	// Server, on possess: fresh hero state (no carried-over effects, full health) and this hero's abilities.
	void ResetHeroState(ABrutusPlayerState& PS);
	void GrantHeroAbilities(UAbilitySystemComponent& ASC);
	void HandleOutOfHealth(AActor* DamageInstigator);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	void ApplyPlaceholderMesh();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusHitboxComponent> Hitbox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusHitscanWeaponComponent> Weapon;

	bool bHiddenForVision = false;
};
