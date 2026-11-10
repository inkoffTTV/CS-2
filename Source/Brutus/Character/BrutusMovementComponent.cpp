#include "Character/BrutusMovementComponent.h"

#include "Core/BrutusSimConvert.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Movement/BrutusZiplineComponent.h"
#include "Movement/BrutusZiplineRegistry.h"

// ---------------------------------------------------------------------------
// Correction payload

void FBrutusMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	FCharacterMoveResponseDataContainer::ServerFillResponseData(CharacterMovement, PendingAdjustment);
	const UBrutusMovementComponent& Move = static_cast<const UBrutusMovementComponent&>(CharacterMovement);
	StaminaCharges = Move.GetStaminaCharges();
	DashTimeRemaining = Move.GetDashTimeRemaining();
}

bool FBrutusMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap)
{
	if (!FCharacterMoveResponseDataContainer::Serialize(CharacterMovement, Ar, PackageMap))
	{
		return false;
	}
	if (IsCorrection())
	{
		Ar << StaminaCharges;
		Ar << DashTimeRemaining;
	}
	return !Ar.IsError();
}

// ---------------------------------------------------------------------------
// Component

UBrutusMovementComponent::UBrutusMovementComponent()
{
	SetMoveResponseDataContainer(BrutusMoveResponseData);

	NavAgentProps.bCanCrouch = true;
	SetCrouchedHalfHeight(50.f);
	MaxWalkSpeed = 650.f;
	JumpZVelocity = 650.f;
	AirControl = 0.4f;
	GravityScale = 1.3f;
	Stamina.Charges = static_cast<float>(StaminaCfg.MaxCharges);
}

bool UBrutusMovementComponent::IsSliding() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(EBrutusMoveMode::Slide);
}

bool UBrutusMovementComponent::IsOnZipline() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(EBrutusMoveMode::Zipline);
}

FNetworkPredictionData_Client* UBrutusMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UBrutusMovementComponent* MutableThis = const_cast<UBrutusMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Brutus(*this);
	}
	return ClientPredictionData;
}

void UBrutusMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToSlide = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToZipline = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
}

void UBrutusMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Crouch capsule follows the slide key (crouch-walk when too slow to slide).
	bWantsToCrouch = bWantsToSlide;

	if (bWantsToZipline)
	{
		bWantsToZipline = false;
		ToggleZipline();
	}
	if (bWantsToDash)
	{
		bWantsToDash = false;
		TryDash();
	}
	TryStartSlide();

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UBrutusMovementComponent::TryDash()
{
	if (IsOnZipline() || !bsim::TryConsume(Stamina, StaminaCfg))
	{
		return;
	}
	const FVector Wish = GetCurrentAcceleration().GetSafeNormal2D();
	const FVector Facing = UpdatedComponent->GetForwardVector();
	Velocity = ToUE(bsim::ComputeDashVelocity(ToSim(Wish), ToSim(Facing), MoveCfg));
	DashTimeRemaining = DashDuration;
	if (IsSliding())
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UBrutusMovementComponent::TryStartSlide()
{
	if (!bWantsToSlide || IsSliding() || !IsMovingOnGround())
	{
		return;
	}
	if (!bsim::CanStartSlide(Velocity.Size2D(), true, MoveCfg))
	{
		return;
	}
	Velocity = ToUE(bsim::StartSlideVelocity(ToSim(Velocity), MoveCfg));
	SetMovementMode(MOVE_Custom, static_cast<uint8>(EBrutusMoveMode::Slide));
}

void UBrutusMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	bsim::Regen(Stamina, StaminaCfg, DeltaSeconds);
	DashTimeRemaining = FMath::Max(0.f, DashTimeRemaining - DeltaSeconds);
}

void UBrutusMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);
	if (CustomMovementMode == static_cast<uint8>(EBrutusMoveMode::Slide))
	{
		PhysSlide(DeltaTime, Iterations);
	}
	else if (CustomMovementMode == static_cast<uint8>(EBrutusMoveMode::Zipline))
	{
		PhysZipline(DeltaTime, Iterations);
	}
}

FVector UBrutusMovementComponent::CableProbe() const
{
	// The character hangs HangOffsetZ below the cable.
	return UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, ZiplineCfg.HangOffsetZ);
}

UBrutusZiplineComponent* UBrutusMovementComponent::FindZiplineNear(float MaxDist) const
{
	const UBrutusZiplineRegistry* Registry = GetWorld() ? GetWorld()->GetSubsystem<UBrutusZiplineRegistry>() : nullptr;
	return Registry ? Registry->FindNearest(CableProbe(), MaxDist) : nullptr;
}

void UBrutusMovementComponent::ToggleZipline()
{
	if (IsOnZipline())
	{
		SetMovementMode(MOVE_Falling);
		return;
	}
	UBrutusZiplineComponent* Zipline = FindZiplineNear(ZiplineCfg.AttachRadius);
	if (!Zipline)
	{
		return;
	}
	CurrentZipline = Zipline;
	ZiplineDir = static_cast<int8>(bsim::ChooseDirection(Zipline->GetWorldSegment(), ToSim(UpdatedComponent->GetForwardVector())));
	if (IsCrouching())
	{
		UnCrouch(false);
	}
	SetMovementMode(MOVE_Custom, static_cast<uint8>(EBrutusMoveMode::Zipline));
}

void UBrutusMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}
	// After a server correction the client may be in zipline mode without a cable reference.
	if (!CurrentZipline.IsValid())
	{
		CurrentZipline = FindZiplineNear(ZiplineCfg.AttachRadius * 2.f);
	}
	if (!CurrentZipline.IsValid())
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	// T is derived from the current position each tick, so corrections cannot desync it.
	const bsim::ZiplineSegment Seg = CurrentZipline->GetWorldSegment();
	const float T = bsim::ProjectT(Seg, ToSim(CableProbe()));
	const float NewT = bsim::AdvanceT(Seg, T, ZiplineDir, ZiplineCfg.Speed, DeltaTime);
	const FVector Target = ToUE(bsim::PointAt(Seg, NewT)) + FVector(0.f, 0.f, ZiplineCfg.HangOffsetZ);

	bJustTeleported = false;
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Target - OldLocation, UpdatedComponent->GetComponentQuat(), true, Hit);
	Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;

	const bool bReachedEnd = (ZiplineDir > 0) ? NewT >= 1.f : NewT <= 0.f;
	if (Hit.bBlockingHit || bReachedEnd)
	{
		SetMovementMode(MOVE_Falling);
	}
}

void UBrutusMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (!IsOnZipline())
	{
		CurrentZipline.Reset();
	}
	if (IsSliding())
	{
		// The base class treats every non-walking mode as airborne: it clears the floor and lets
		// crouching shrink the capsule around its centre. A slide is on the ground.
		bCrouchMaintainsBaseLocation = true;
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
	}
}

void UBrutusMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CurrentFloor.IsWalkableFloor())
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	Velocity = ToUE(bsim::StepSlide(ToSim(Velocity), ToSim(CurrentFloor.HitResult.ImpactNormal), DeltaTime, MoveCfg));

	bJustTeleported = false;
	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, DeltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal);
	}

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
	if (!CurrentFloor.IsWalkableFloor())
	{
		SetMovementMode(MOVE_Falling);
		return;
	}
	AdjustFloorHeight();

	if (!bWantsToSlide || bsim::ShouldExitSlide(ToSim(Velocity), MoveCfg))
	{
		SetMovementMode(MOVE_Walking);
	}
}

float UBrutusMovementComponent::GetMaxSpeed() const
{
	if (IsDashing())
	{
		return MoveCfg.DashSpeed;
	}
	if (IsSliding())
	{
		return MoveCfg.SlideMaxSpeed;
	}
	return Super::GetMaxSpeed();
}

float UBrutusMovementComponent::GetMaxBrakingDeceleration() const
{
	if (IsDashing() || IsSliding())
	{
		return 0.f;
	}
	return Super::GetMaxBrakingDeceleration();
}

bool UBrutusMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() || (CanEverCrouch() && IsSliding());
}

bool UBrutusMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsSliding();
}

void UBrutusMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	// During a dash neither friction nor braking may eat the dash speed, even with no input.
	if (IsDashing())
	{
		Super::CalcVelocity(DeltaTime, 0.f, bFluid, 0.f);
		return;
	}
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}

bool UBrutusMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling() || IsSliding() || IsOnZipline());
}

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
bool UBrutusMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
#else
bool UBrutusMovementComponent::DoJump(bool bReplayingMoves)
#endif
{
	// Air jumps cost a stamina charge; the jump count limit comes from ACharacter::JumpMaxCount.
	if (IsFalling() && !bsim::TryConsume(Stamina, StaminaCfg))
	{
		return false;
	}
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
	const bool bJumped = Super::DoJump(bReplayingMoves, DeltaTime);
#else
	const bool bJumped = Super::DoJump(bReplayingMoves);
#endif
	if (bJumped && IsFalling())
	{
		// An air jump resets downward speed so it always gains height.
		Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, JumpZVelocity);
	}
	return bJumped;
}

void UBrutusMovementComponent::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse)
{
	if (MoveResponse.IsCorrection())
	{
		const FBrutusMoveResponseDataContainer& Data = static_cast<const FBrutusMoveResponseDataContainer&>(MoveResponse);
		Stamina.Charges = Data.StaminaCharges;
		DashTimeRemaining = Data.DashTimeRemaining;
	}
	Super::ClientHandleMoveResponse(MoveResponse);
}

// ---------------------------------------------------------------------------
// Saved moves

void FSavedMove_Brutus::Clear()
{
	Super::Clear();
	bSavedWantsToDash = 0;
	bSavedWantsToSlide = 0;
	bSavedWantsToZipline = 0;
	SavedZiplineDir = 1;
	SavedZipline.Reset();
}

uint8 FSavedMove_Brutus::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsToDash)
	{
		Flags |= FLAG_Custom_0;
	}
	if (bSavedWantsToSlide)
	{
		Flags |= FLAG_Custom_1;
	}
	if (bSavedWantsToZipline)
	{
		Flags |= FLAG_Custom_2;
	}
	return Flags;
}

bool FSavedMove_Brutus::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Brutus* Other = static_cast<const FSavedMove_Brutus*>(NewMove.Get());
	if (bSavedWantsToDash || Other->bSavedWantsToDash
		|| bSavedWantsToSlide != Other->bSavedWantsToSlide
		|| bSavedWantsToZipline != Other->bSavedWantsToZipline)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Brutus::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (const UBrutusMovementComponent* Move = Cast<UBrutusMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToDash = Move->bWantsToDash;
		bSavedWantsToSlide = Move->bWantsToSlide;
		bSavedWantsToZipline = Move->bWantsToZipline;
		SavedZiplineDir = Move->ZiplineDir;
		SavedZipline = Move->CurrentZipline;
	}
}

void FSavedMove_Brutus::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	if (UBrutusMovementComponent* Move = Cast<UBrutusMovementComponent>(C->GetCharacterMovement()))
	{
		Move->bWantsToDash = bSavedWantsToDash;
		Move->bWantsToSlide = bSavedWantsToSlide;
		Move->bWantsToZipline = bSavedWantsToZipline;
		Move->ZiplineDir = SavedZiplineDir;
		Move->CurrentZipline = SavedZipline;
	}
}

FNetworkPredictionData_Client_Brutus::FNetworkPredictionData_Client_Brutus(const UCharacterMovementComponent& ClientMovement)
	: FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Brutus::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Brutus());
}
