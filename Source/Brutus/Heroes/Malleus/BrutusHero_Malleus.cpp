#include "Heroes/Malleus/BrutusHero_Malleus.h"

#include "Combat/BrutusHitscanWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Heroes/Malleus/BrutusMalleusChainAbility.h"
#include "Heroes/Malleus/BrutusMalleusGroundSlamAbility.h"
#include "Heroes/Malleus/BrutusMalleusHammerAbility.h"
#include "Heroes/Malleus/BrutusMalleusRageAbility.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABrutusHero_Malleus::ABrutusHero_Malleus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Starting values for tuning.
	GetCapsuleComponent()->InitCapsuleSize(60.f, 105.f);
	HeroMaxHealth = 400.f;

	// Skeletal mesh origin at the feet (the usual UE setup), so scaling the visuals alone (Rage without
	// room for a bigger capsule) grows them upwards from the floor. FitBodyMeshToCapsule cancels this offset
	// for the body cylinder.
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -105.f));
	FitBodyMeshToCapsule();

	// The wider body would cover the crosshair with the default shoulder camera: move the camera out a bit.
	if (USpringArmComponent* Boom = Cast<USpringArmComponent>(GetDefaultSubobjectByName(TEXT("CameraBoom"))))
	{
		Boom->TargetArmLength = 340.f;
		Boom->SocketOffset = FVector(0.f, 90.f, 75.f);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	auto MakeGear = [this](const TCHAR* Name, UStaticMesh* MeshAsset)
	{
		UStaticMeshComponent* Gear = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		// Attached like the body cylinder, so network smoothing moves it too.
		Gear->SetupAttachment(GetMesh());
		Gear->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gear->SetGenerateOverlapEvents(false);
		Gear->SetCanEverAffectNavigation(false);
		if (MeshAsset)
		{
			Gear->SetStaticMesh(MeshAsset);
		}
		return Gear;
	};
	UStaticMesh* CubeMesh = Cube.Object; // null when not found
	UStaticMesh* CylinderMesh = Cylinder.Object;
	HammerHead = MakeGear(TEXT("HammerHead"), CubeMesh);
	HammerHandle = MakeGear(TEXT("HammerHandle"), CylinderMesh);
	Shoulders = MakeGear(TEXT("Shoulders"), CubeMesh);
	FitGearToCapsule();
}

TArray<TSubclassOf<UBrutusGameplayAbility>> ABrutusHero_Malleus::GetHeroAbilities() const
{
	TArray<TSubclassOf<UBrutusGameplayAbility>> Abilities;
	Abilities.Add(UBrutusMalleusHammerAbility::StaticClass());
	Abilities.Add(UBrutusMalleusGroundSlamAbility::StaticClass());
	Abilities.Add(UBrutusMalleusChainAbility::StaticClass());
	Abilities.Add(UBrutusMalleusRageAbility::StaticClass());
	return Abilities;
}

void ABrutusHero_Malleus::ConfigureWeapon(UBrutusHitscanWeaponComponent& InWeapon)
{
	// Shotgun. Starting values for tuning.
	InWeapon.Pellets = 8;
	InWeapon.Damage = 9.f;
	InWeapon.SpreadDegrees = 5.f;
	InWeapon.FireRate = 1.4f;
}

void ABrutusHero_Malleus::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusHero_Malleus, RageScale);
}

void ABrutusHero_Malleus::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	FitGearToCapsule();
}

void ABrutusHero_Malleus::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitGearToCapsule();
}

void ABrutusHero_Malleus::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitGearToCapsule();
}

void ABrutusHero_Malleus::FitGearToCapsule()
{
	if (!HammerHead || !HammerHandle || !Shoulders)
	{
		return;
	}
	// Engine cube and cylinder are 100 cm, centred on their pivots. Offsets are from the capsule centre;
	// the mesh's own offset from the capsule is cancelled (as FitBodyMeshToCapsule does for the body).
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float MeshZ = GetMesh()->GetRelativeLocation().Z;
	const float SideY = Radius + 30.f; // right hand side

	// The hammer hangs head-down from the right hand, low, so it stays well below the crosshair.
	const float HeadCenterZ = -0.6f * HalfHeight;
	const float HandleTopZ = 0.1f * HalfHeight;
	HammerHead->SetRelativeLocation(FVector(20.f, SideY, HeadCenterZ - MeshZ));
	HammerHead->SetRelativeScale3D(FVector(1.0f, 0.6f, 0.6f));
	HammerHandle->SetRelativeLocation(FVector(20.f, SideY, (HeadCenterZ + HandleTopZ) * 0.5f - MeshZ));
	HammerHandle->SetRelativeScale3D(FVector(0.12f, 0.12f, (HandleTopZ - HeadCenterZ) / 100.f));

	// Shoulders: a slab wider than the capsule (but inside the camera's shoulder offset).
	Shoulders->SetRelativeLocation(FVector(0.f, 0.f, 0.6f * HalfHeight - MeshZ));
	Shoulders->SetRelativeScale3D(FVector(Radius * 1.6f / 100.f, (Radius * 2.f + 30.f) / 100.f, 0.35f));
}

// ---------------------------------------------------------------------------
// Rage size

void ABrutusHero_Malleus::SetRageActive(bool bActive)
{
	const EBrutusMalleusRageScale Wanted = !bActive
		? EBrutusMalleusRageScale::None
		: (AppliedRageScale != EBrutusMalleusRageScale::None ? AppliedRageScale : ChooseRageScale());

	if (HasAuthority())
	{
		RageScale = Wanted;
		ApplyRageScale(Wanted);
		ForceNetUpdate();
	}
	else if (IsLocallyControlled())
	{
		// Predict the start; on the end fall back to whatever the server says now (OnRep finishes the job).
		ApplyRageScale(bActive ? Wanted : RageScale);
	}
}

void ABrutusHero_Malleus::OnRep_RageScale()
{
	ApplyRageScale(RageScale);
}

float ABrutusHero_Malleus::GetRageLift() const
{
	return GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * (RageScaleFactor - 1.f);
}

EBrutusMalleusRageScale ABrutusHero_Malleus::ChooseRageScale() const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return EBrutusMalleusRageScale::VisualOnly;
	}
	// The grown capsule is raised by the lift so its bottom stays where the feet are.
	const float Tolerance = 1.f;
	const float Radius = Capsule->GetUnscaledCapsuleRadius() * RageScaleFactor - Tolerance;
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight() * RageScaleFactor - Tolerance;
	const FVector Center = GetActorLocation() + FVector(0.f, 0.f, GetRageLift());

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusMalleusRageRoom), false, this);
	FCollisionResponseParams Response;
	Capsule->InitSweepCollisionParams(Params, Response);
	const bool bBlocked = World->OverlapBlockingTestByChannel(Center, GetActorQuat(), Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(Radius, HalfHeight), Params, Response);
	return bBlocked ? EBrutusMalleusRageScale::VisualOnly : EBrutusMalleusRageScale::Full;
}

void ABrutusHero_Malleus::ApplyRageScale(EBrutusMalleusRageScale NewScale)
{
	if (NewScale == AppliedRageScale)
	{
		return;
	}
	// Only the machines that simulate this hero's movement move it; simulated proxies get positions from the server.
	const bool bMovesSelf = HasAuthority() || IsLocallyControlled();
	const float Lift = GetRageLift();

	if (AppliedRageScale == EBrutusMalleusRageScale::Full)
	{
		SetActorScale3D(FVector::OneVector);
		const UCharacterMovementComponent* Move = GetCharacterMovement();
		if (bMovesSelf && Move && Move->IsMovingOnGround())
		{
			// Shrinking about the centre lifts the feet off the floor: sweep back down onto it.
			AddActorWorldOffset(FVector(0.f, 0.f, -Lift), true);
		}
	}
	else if (AppliedRageScale == EBrutusMalleusRageScale::VisualOnly)
	{
		GetMesh()->SetRelativeScale3D(FVector::OneVector);
	}

	if (NewScale == EBrutusMalleusRageScale::Full)
	{
		// Scaling the root scales the capsule (and so the lag-compensation hitboxes), the body and the gear.
		// The CMC works with the scaled capsule, crouching included. Raise first so the feet stay on the floor.
		if (bMovesSelf)
		{
			AddActorWorldOffset(FVector(0.f, 0.f, Lift), false, nullptr, ETeleportType::TeleportPhysics);
		}
		SetActorScale3D(FVector(RageScaleFactor));
	}
	else if (NewScale == EBrutusMalleusRageScale::VisualOnly)
	{
		// The mesh origin is at the feet, so the body and the gear grow upwards.
		GetMesh()->SetRelativeScale3D(FVector(RageScaleFactor));
	}

	AppliedRageScale = NewScale;
}
