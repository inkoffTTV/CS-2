#include "Character/BrutusCharacter.h"

#include "Abilities/BrutusAttributeSet.h"
#include "Abilities/BrutusCooldownEffect.h"
#include "Abilities/BrutusStunEffect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Camera/CameraComponent.h"
#include "Character/BrutusHitboxComponent.h"
#include "Character/BrutusMovementComponent.h"
#include "Combat/BrutusHitscanWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Core/BrutusGameMode.h"
#include "Core/BrutusPlayerController.h"
#include "Core/BrutusPlayerState.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"
#include "Vision/BrutusTeamVisionSubsystem.h"

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
	BodyMesh->ComponentTags.Add(GreyboxTag);

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(GetMesh());
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetGenerateOverlapEvents(false);
	PlaceholderMesh->SetCanEverAffectNavigation(false);
	PlaceholderMesh->SetVisibility(false);

	HeroBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeroBody"));
	HeroBody->SetupAttachment(GetMesh());
	HeroBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeroBody->SetGenerateOverlapEvents(false);
	HeroBody->SetCanEverAffectNavigation(false);
	HeroBody->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	HeroBody->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	HeroBody->SetVisibility(false);

	HeroWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeroWeapon"));
	HeroWeapon->SetupAttachment(HeroBody, TEXT("weapon_r"));
	HeroWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeroWeapon->SetGenerateOverlapEvents(false);
	HeroWeapon->SetCanEverAffectNavigation(false);
	HeroWeapon->SetVisibility(false);
	// Attached to the skeletal mesh so CMC network smoothing (which offsets it) also moves the body on simulated proxies.
	BodyMesh->SetupAttachment(GetMesh());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(Cylinder.Object);
	}
	FitBodyMeshToCapsule();

	Hitbox = CreateDefaultSubobject<UBrutusHitboxComponent>(TEXT("Hitbox"));
	Weapon = CreateDefaultSubobject<UBrutusHitscanWeaponComponent>(TEXT("Weapon"));

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

void ABrutusCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Hero constructors may have resized the capsule after the base constructor fitted the body.
	FitBodyMeshToCapsule();
	if (Weapon)
	{
		ConfigureWeapon(*Weapon);
	}
}

void ABrutusCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilitySystem();
	if (ABrutusPlayerState* PS = GetBrutusPlayerState())
	{
		PS->GetAttributes()->OnOutOfHealth.AddUObject(this, &ABrutusCharacter::HandleOutOfHealth);
		GrantHeroAbilities(*PS->GetAbilitySystemComponent());
		ResetHeroState(*PS);
	}
	NotifyTeleported();
}

void ABrutusCharacter::UnPossessed()
{
	// The player state is cleared by Super; stop listening to its health first.
	if (ABrutusPlayerState* PS = GetBrutusPlayerState())
	{
		PS->GetAttributes()->OnOutOfHealth.RemoveAll(this);
	}
	Super::UnPossessed();
}

void ABrutusCharacter::GrantHeroAbilities(UAbilitySystemComponent& ASC)
{
	// The ability system lives on the player state: drop the previous hero's (or life's) abilities first.
	ASC.ClearAllAbilities();
	for (const TSubclassOf<UBrutusGameplayAbility>& AbilityClass : GetHeroAbilities())
	{
		UBrutusGameplayAbility* Ability = AbilityClass ? AbilityClass.GetDefaultObject() : nullptr;
		if (!Ability)
		{
			continue;
		}
		const int32 InputId = Ability->InputId == EBrutusAbilityInput::None ? INDEX_NONE : static_cast<int32>(Ability->InputId);
		ASC.GiveAbility(FGameplayAbilitySpec(Ability, 1, InputId, this));
	}
}

void ABrutusCharacter::ResetHeroState(ABrutusPlayerState& PS)
{
	UAbilitySystemComponent* ASC = PS.GetAbilitySystemComponent();

	// Timed effects of the previous life (stun, slows, buffs) must not carry over.
	// Cooldowns survive a plain respawn and are cleared only when the hero changes.
	// Infinite effects (future items) stay with the player.
	const bool bResetCooldowns = PS.bResetCooldownsOnNextSpawn;
	PS.bResetCooldownsOnNextSpawn = false;
	if (bResetCooldowns)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(UBrutusCooldownEffect::StaticClass(), nullptr);
	}
	ASC->RemoveActiveGameplayEffectBySourceEffect(UBrutusStunEffect::StaticClass(), nullptr);
	for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
		const UGameplayEffect* Def = Effect ? Effect->Spec.Def.Get() : nullptr;
		if (Def && Def->DurationPolicy == EGameplayEffectDurationType::HasDuration && !Def->IsA(UBrutusCooldownEffect::StaticClass()))
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	ASC->SetNumericAttributeBase(UBrutusAttributeSet::GetMaxHealthAttribute(), HeroMaxHealth);
	ASC->SetNumericAttributeBase(UBrutusAttributeSet::GetHealthAttribute(), HeroMaxHealth);
	ASC->SetNumericAttributeBase(UBrutusAttributeSet::GetMoveSpeedMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UBrutusAttributeSet::GetDamageTakenMultiplierAttribute(), 1.f);
	PS.GetAttributes()->ResetOutOfHealth();
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

const FName ABrutusCharacter::GreyboxTag(TEXT("Greybox"));

void ABrutusCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyHeroArt();
}

void ABrutusCharacter::SetHeroArtPaths(const TCHAR* HeroName, const TCHAR* WeaponName)
{
	const FString Dir = FString::Printf(TEXT("/Game/Characters/%s/"), HeroName);
	PlaceholderMeshPath = FSoftObjectPath(FString::Printf(TEXT("%sSM_Hero_%s_Posed.SM_Hero_%s_Posed"), *Dir, HeroName, HeroName));
	SkeletalMeshPath = FSoftObjectPath(FString::Printf(TEXT("%sSK_Hero_%s.SK_Hero_%s"), *Dir, HeroName, HeroName));
	AnimClassPath = FSoftClassPath(FString::Printf(TEXT("%sABP_%s.ABP_%s_C"), *Dir, HeroName, HeroName));
	const FString Weapon = FString::Printf(TEXT("SM_Wpn_%s_%s"), HeroName, WeaponName);
	WeaponMeshPath = FSoftObjectPath(FString::Printf(TEXT("/Game/Weapons/%s/%s.%s"), HeroName, *Weapon, *Weapon));
}

void ABrutusCharacter::ApplyHeroArt()
{
	// Quiet loads throughout: until the author imports the art, missing assets are expected, not errors.
	if (TryApplyAnimatedArt() || TryApplyPlaceholderArt())
	{
		HideGreybox();
	}
}

bool ABrutusCharacter::TryApplyAnimatedArt()
{
	if (SkeletalMeshPath.IsNull() || AnimClassPath.IsNull())
	{
		return false;
	}
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *SkeletalMeshPath.ToString(), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, *AnimClassPath.ToString(), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Body || !AnimClass)
	{
		return false;
	}
	HeroBody->SetSkeletalMeshAsset(Body);
	HeroBody->SetAnimInstanceClass(AnimClass);
	HeroBody->SetVisibility(true);

	if (!WeaponMeshPath.IsNull())
	{
		if (UStaticMesh* Gun = LoadObject<UStaticMesh>(nullptr, *WeaponMeshPath.ToString(), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			HeroWeapon->SetStaticMesh(Gun);
			// Re-attach now that the skeleton (and its weapon_r bone) exists.
			HeroWeapon->AttachToComponent(HeroBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
			HeroWeapon->SetVisibility(true);
		}
	}
	return true;
}

bool ABrutusCharacter::TryApplyPlaceholderArt()
{
	if (PlaceholderMeshPath.IsNull())
	{
		return false;
	}
	UStaticMesh* Model = LoadObject<UStaticMesh>(nullptr, *PlaceholderMeshPath.ToString(), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Model)
	{
		return false;
	}
	PlaceholderMesh->SetStaticMesh(Model);
	PlaceholderMesh->SetVisibility(true);
	return true;
}

void ABrutusCharacter::HideGreybox()
{
	TInlineComponentArray<UStaticMeshComponent*> Parts(this);
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part->ComponentHasTag(GreyboxTag))
		{
			Part->SetVisibility(false);
		}
	}
}

void ABrutusCharacter::FitBodyMeshToCapsule()
{
	// Engine cylinder is 100 x 100 x 100 cm, centred on its pivot.
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	BodyMesh->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, HalfHeight * 2.f / 100.f));
	// ACharacter shifts the skeletal mesh relative to the capsule (base offset, crouch half-height adjust);
	// cancel that shift so the cylinder stays centred on the capsule. The mesh has no rotation or scale here.
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -GetMesh()->GetRelativeLocation().Z));
	// The placeholder model's pivot is at its feet: keep it on the capsule bottom (standing pose even when crouched).
	const FVector Feet(0.f, 0.f, -GetMesh()->GetRelativeLocation().Z - HalfHeight);
	PlaceholderMesh->SetRelativeLocation(Feet);
	HeroBody->SetRelativeLocation(Feet);
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
	Input->BindAction(PC->GetAction(EBrutusInput::Fire), ETriggerEvent::Started, this, &ABrutusCharacter::OnFireStarted);
	Input->BindAction(PC->GetAction(EBrutusInput::Fire), ETriggerEvent::Completed, this, &ABrutusCharacter::OnFireCompleted);

	// Ability keys go to the ability system with the slot as GAS InputID.
	auto BindAbilityInput = [this, Input, PC](EBrutusInput Action, EBrutusAbilityInput AbilityInput)
	{
		Input->BindAction(PC->GetAction(Action), ETriggerEvent::Started, this, &ABrutusCharacter::OnAbilityInputPressed, AbilityInput);
		Input->BindAction(PC->GetAction(Action), ETriggerEvent::Completed, this, &ABrutusCharacter::OnAbilityInputReleased, AbilityInput);
	};
	BindAbilityInput(EBrutusInput::Ability1, EBrutusAbilityInput::Ability1);
	BindAbilityInput(EBrutusInput::Ability2, EBrutusAbilityInput::Ability2);
	BindAbilityInput(EBrutusInput::Ultimate, EBrutusAbilityInput::Ultimate);
	BindAbilityInput(EBrutusInput::AltFire, EBrutusAbilityInput::AltFire);
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

void ABrutusCharacter::OnFireStarted()
{
	Weapon->StartFire();
}

void ABrutusCharacter::OnFireCompleted()
{
	Weapon->StopFire();
}

void ABrutusCharacter::OnAbilityInputPressed(EBrutusAbilityInput InputId)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AbilityLocalInputPressed(static_cast<int32>(InputId));
	}
}

void ABrutusCharacter::OnAbilityInputReleased(EBrutusAbilityInput InputId)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AbilityLocalInputReleased(static_cast<int32>(InputId));
	}
}

bool ABrutusCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (!Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation))
	{
		return false;
	}
	const APlayerController* ViewerPC = Cast<APlayerController>(RealViewer);
	const ABrutusPlayerState* ViewerPS = ViewerPC ? ViewerPC->GetPlayerState<ABrutusPlayerState>() : nullptr;
	if (!ViewerPS || ViewerPS->GetTeamId() == GetTeamId())
	{
		return true;
	}
	const UBrutusTeamVisionSubsystem* VisionSubsystem = GetWorld()->GetSubsystem<UBrutusTeamVisionSubsystem>();
	return !VisionSubsystem || VisionSubsystem->IsVisibleToTeam(this, ViewerPS->GetTeamId());
}
