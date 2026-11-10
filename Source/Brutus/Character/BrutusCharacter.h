#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BrutusCharacter.generated.h"

class UBrutusMovementComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

// Greybox hero: capsule + cylinder, over-the-shoulder camera, Brutus movement.
UCLASS()
class BRUTUS_API ABrutusCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	explicit ABrutusCharacter(const FObjectInitializer& ObjectInitializer);

	UBrutusMovementComponent* GetBrutusMovement() const;

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

	void FitBodyMeshToCapsule();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BodyMesh;
};
