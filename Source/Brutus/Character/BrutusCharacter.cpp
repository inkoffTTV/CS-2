#include "Character/BrutusCharacter.h"

#include "Abilities/BrutusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/BrutusMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/BrutusGameMode.h"
#include "Core/BrutusPlayerController.h"
#include "Core/BrutusPlayerState.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"

ABrutusCharacter::ABrutusCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBrutusMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	JumpMaxCount = 2; // ground jump + one air jump (paid with stamina)

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(Cylinder.Object);
	}
	FitBodyMeshToCapsule();

	// The skeletal mesh is unused in the greybox.
	GetMesh()->SetVisibility(false);
}

UBrutusMovementComponent* ABrutusCharacter::GetBrutusMovement() const
{
	return Cast<UBrutusMovementComponent>(GetCharacterMovement());
}

ABrutusPlayerState* ABrutusCharacter::GetBrutusPlayerState() const
{
	return GetPlayerState<ABrutusPlayerState>();
}

UAbilitySystemComponent* ABrutusCharacter::GetAbilitySystemComponent() const
{
	const ABrutusPlayerState* PS = GetBrutusPlayerState();
	return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

uint8 ABrutusCharacter::GetTeamId() const
{
	const ABrutusPlayerState* PS = GetBrutusPlayerState();
	return PS ? PS->GetTeamId() : 255;
}

void ABrutusCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilitySystem();
	if (ABrutusPlayerState* PS = GetBrutusPlayerState())
	{
		PS->GetAttributes()->OnOutOfHealth.AddUObject(this, &ABrutusCharacter::HandleOutOfHealth);
	}
	NotifyTeleported();
}

void ABrutusCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilitySystem();
}

void ABrutusCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ABrutusPlayerState* PS = GetBrutusPlayerState())
	{
		PS->GetAttributes()->OnOutOfHealth.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ABrutusCharacter::InitAbilitySystem()
{
	if (ABrutusPlayerState* PS = GetBrutusPlayerState())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}

void ABrutusCharacter::HandleOutOfHealth(AActor* DamageInstigator)
{
	AController* Killer = nullptr;
	if (const APlayerState* KillerPS = Cast<APlayerState>(DamageInstigator))
	{
		Killer = KillerPS->GetOwningController();
	}
	else if (const APawn* KillerPawn = Cast<APawn>(DamageInstigator))
	{
		Killer = KillerPawn->GetController();
	}
	if (ABrutusGameMode* GM = GetWorld()->GetAuthGameMode<ABrutusGameMode>())
	{
		GM->HandleHeroDeath(this, Killer);
	}
}

void ABrutusCharacter::FitBodyMeshToCapsule()
{
	// Engine cylinder is 100 x 100 x 100 cm, centred on its pivot.
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	BodyMesh->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, HalfHeight * 2.f / 100.f));
}

void ABrutusCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitBodyMeshToCapsule();
}

void ABrutusCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitBodyMeshToCapsule();
}

bool ABrutusCharacter::CanJumpInternal_Implementation() const
{
	// Crouching normally blocks jumping; jumping out of a slide is allowed.
	const UBrutusMovementComponent* Move = GetBrutusMovement();
	return (!bIsCrouched || (Move && Move->IsSliding())) && JumpIsAllowedInternal();
}

void ABrutusCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const ABrutusPlayerController* PC = Cast<ABrutusPlayerController>(GetController());
	if (!Input || !PC)
	{
		return;
	}

	Input->BindAction(PC->GetAction(EBrutusInput::Move), ETriggerEvent::Triggered, this, &ABrutusCharacter::OnMove);
	Input->BindAction(PC->GetAction(EBrutusInput::Look), ETriggerEvent::Triggered, this, &ABrutusCharacter::OnLook);
	Input->BindAction(PC->GetAction(EBrutusInput::Jump), ETriggerEvent::Started, this, &ABrutusCharacter::OnJumpStarted);
	Input->BindAction(PC->GetAction(EBrutusInput::Jump), ETriggerEvent::Completed, this, &ABrutusCharacter::OnJumpCompleted);
	Input->BindAction(PC->GetAction(EBrutusInput::Dash), ETriggerEvent::Started, this, &ABrutusCharacter::OnDash);
	Input->BindAction(PC->GetAction(EBrutusInput::Slide), ETriggerEvent::Started, this, &ABrutusCharacter::OnSlideStarted);
	Input->BindAction(PC->GetAction(EBrutusInput::Slide), ETriggerEvent::Completed, this, &ABrutusCharacter::OnSlideCompleted);
	Input->BindAction(PC->GetAction(EBrutusInput::Interact), ETriggerEvent::Started, this, &ABrutusCharacter::OnInteract);
}

void ABrutusCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ABrutusCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ABrutusCharacter::OnJumpStarted()
{
	Jump();
}

void ABrutusCharacter::OnJumpCompleted()
{
	StopJumping();
}

void ABrutusCharacter::OnDash()
{
	if (UBrutusMovementComponent* Move = GetBrutusMovement())
	{
		Move->bWantsToDash = true;
	}
}

void ABrutusCharacter::OnSlideStarted()
{
	if (UBrutusMovementComponent* Move = GetBrutusMovement())
	{
		Move->bWantsToSlide = true;
	}
}

void ABrutusCharacter::OnSlideCompleted()
{
	if (UBrutusMovementComponent* Move = GetBrutusMovement())
	{
		Move->bWantsToSlide = false;
	}
}

void ABrutusCharacter::OnInteract()
{
	if (UBrutusMovementComponent* Move = GetBrutusMovement())
	{
		Move->bWantsToZipline = true; // one-shot: attach, or detach when already riding
	}
}
