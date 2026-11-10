#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "MoveRules.h"
#include "Stamina.h"
#include "Zipline.h"
#include "BrutusMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EBrutusMoveMode : uint8
{
	None = 0 UMETA(Hidden),
	Slide = 1,
	Zipline = 2,
};

// Server -> client correction payload: predicted state that is not part of the base move.
struct FBrutusMoveResponseDataContainer : public FCharacterMoveResponseDataContainer
{
	float StaminaCharges = 0.f;
	float DashTimeRemaining = 0.f;

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;
};

// Hero movement: stamina, dash, stamina-paid air jump, slide, zipline. All client-predicted.
UCLASS()
class BRUTUS_API UBrutusMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_Brutus;

public:
	UBrutusMovementComponent();

	// Input-side wishes, sent to the server as compressed flags.
	bool bWantsToDash = false;
	bool bWantsToSlide = false;
	bool bWantsToZipline = false;

	float GetStaminaCharges() const { return Stamina.Charges; }
	int32 GetMaxStaminaCharges() const { return StaminaCfg.MaxCharges; }
	bool IsSliding() const;
	bool IsOnZipline() const;
	bool IsDashing() const { return DashTimeRemaining > 0.f; }
	float GetDashTimeRemaining() const { return DashTimeRemaining; }

	// UCharacterMovementComponent
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual bool CanAttemptJump() const override;
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
#else
	virtual bool DoJump(bool bReplayingMoves) override;
#endif
	virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse) override;

protected:
	bsim::StaminaConfig StaminaCfg;
	bsim::StaminaState Stamina;
	bsim::MoveConfig MoveCfg;

	// Seconds of dash left; while > 0 the dash speed is not braked.
	float DashTimeRemaining = 0.f;
	float DashDuration = 0.2f;

private:
	void TryDash();
	void TryStartSlide();
	void PhysSlide(float DeltaTime, int32 Iterations);

	FBrutusMoveResponseDataContainer BrutusMoveResponseData;
};

class FSavedMove_Brutus : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;

	uint8 bSavedWantsToDash : 1;
	uint8 bSavedWantsToSlide : 1;
	uint8 bSavedWantsToZipline : 1;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
};

class FNetworkPredictionData_Client_Brutus : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_Brutus(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};
