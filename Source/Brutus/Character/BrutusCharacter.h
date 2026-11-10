#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
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

	// Invisible heroes (Nemo) are only revealed to enemies up close.
	bool IsHiddenForVision() const { return bHiddenForVision; }
	void SetHiddenForVision(bool bHidden) { bHiddenForVision = bHidden; }

	// Enemies the viewer's team cannot see are not replicated to that viewer.
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

protected:
	virtual bool CanJumpInternal_Implementation() const override;

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

	void FitBodyMeshToCapsule();
	void InitAbilitySystem();
	void HandleOutOfHealth(AActor* DamageInstigator);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusHitboxComponent> Hitbox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusHitscanWeaponComponent> Weapon;

	bool bHiddenForVision = false;
};
